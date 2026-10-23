package com.gazeora.gazeora;

import android.content.Context;
import android.content.Intent;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;

import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Plays the PC's stream full screen, over whichever protocol Gazeora streams with, through the
 * native player ({@link NativePlayer}): frames are decoded straight onto the surface and shown
 * as soon as they're decoded.
 */
public class PlayerActivity extends AppCompatActivity implements NativePlayer.Listener {
    private static final String EXTRA_PROTOCOL = "protocol";
    private static final String EXTRA_HOST = "host";
    private static final String EXTRA_PORT = "port";
    private static final String EXTRA_PATH = "path";
    private static final String EXTRA_NAME = "name";

    private StreamProtocol protocol;
    private String host;
    private int port;
    private String path;
    private String where;  // the PC, for people

    private AspectRatioLayout frameLayout;
    private View status;
    private View progress;
    private TextView statusText;
    private TextView statusDetail;
    private View retry;

    private boolean started;
    private Surface surface;
    private NativePlayer player;
    private UsbAccessoryWatcher usbWatcher;
    private WifiManager.WifiLock wifiLock;
    // Following the PC when its protocol changes: its name, and the PCs that said they stream
    // over USB (by address and name).
    private Discovery discovery;
    @Nullable private String pcName;
    private final Set<String> usbPcs = new HashSet<>();
    private boolean pictureShown;

    /** Opens the stream; {@code host} and {@code port} aren't needed for USB. */
    static Intent intent(Context context, StreamProtocol protocol, @Nullable String host,
            int port, @Nullable String path, @Nullable String name) {
        return new Intent(context, PlayerActivity.class)
                .putExtra(EXTRA_PROTOCOL, protocol.id)
                .putExtra(EXTRA_HOST, host)
                .putExtra(EXTRA_PORT, port)
                .putExtra(EXTRA_PATH, path)
                .putExtra(EXTRA_NAME, name);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Intent intent = getIntent();
        protocol = StreamProtocol.fromId(intent.getStringExtra(EXTRA_PROTOCOL));
        if (protocol == null) {
            protocol = StreamProtocol.RTSP_TCP;
        }
        host = intent.getStringExtra(EXTRA_HOST);
        port = intent.getIntExtra(EXTRA_PORT, 0);
        path = intent.getStringExtra(EXTRA_PATH);
        String name = intent.getStringExtra(EXTRA_NAME);
        where = name != null ? name
                : protocol.needsAddress() ? host + ":" + port : getString(R.string.protocol_usb);

        setContentView(R.layout.activity_player);
        frameLayout = findViewById(R.id.frame_layout);
        status = findViewById(R.id.status);
        progress = findViewById(R.id.progress);
        statusText = findViewById(R.id.status_text);
        statusDetail = findViewById(R.id.status_detail);
        retry = findViewById(R.id.retry);
        retry.setOnClickListener(v -> play());
        pcName = name;
        discovery = new Discovery(MainActivity.isEmulator(), this::onPcsChanged);
        // Wi-Fi power saving holds packets back and drops some, which costs whole frames.
        WifiManager wifi = (WifiManager) getApplicationContext().getSystemService(WIFI_SERVICE);
        wifiLock = wifi.createWifiLock(WifiManager.WIFI_MODE_FULL_LOW_LATENCY, "Gazeora:stream");
        wifiLock.setReferenceCounted(false);

        SurfaceView frameSurface = findViewById(R.id.frame_surface);
        frameSurface.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(@NonNull SurfaceHolder holder) {
                surface = holder.getSurface();
                if (started) {
                    startPlayer();
                }
            }

            @Override
            public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width,
                    int height) {
            }

            @Override
            public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
                stopPlayer();
                surface = null;
            }
        });

        // Every pixel for the picture: over the cutout, under hidden system bars.
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        getWindow().getAttributes().layoutInDisplayCutoutMode =
                WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
        WindowCompat.setDecorFitsSystemWindows(getWindow(), false);
        WindowInsetsControllerCompat bars =
                WindowCompat.getInsetsController(getWindow(), getWindow().getDecorView());
        bars.setSystemBarsBehavior(
                WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
        bars.hide(WindowInsetsCompat.Type.systemBars());
    }

    @Override
    protected void onStart() {
        super.onStart();
        started = true;
        play();
        if (MainActivity.hasLocalNetwork(this)) {
            discovery.start();
        }
    }

    @Override
    protected void onStop() {
        super.onStop();
        // Stop streaming while out of sight; onStart reconnects.
        started = false;
        stopPlayer();
        discovery.stop();
    }

    /**
     * Follows the PC to another protocol: changing it in Gazeora restarts the stream with it,
     * and the PC then says so when the app searches the network. The PC is the one this plays
     * from (by name, or by address when entered by hand); for USB, one that said it streams
     * over USB.
     */
    private void onPcsChanged(List<Discovery.Pc> pcs) {
        Discovery.Pc moved = null;
        for (Discovery.Pc pc : pcs) {
            String key = pc.host + " " + pc.name;
            if (protocol == StreamProtocol.USB) {
                if (pc.protocol == StreamProtocol.USB) {
                    usbPcs.add(key);
                } else if (usbPcs.contains(key)) {
                    moved = pc;
                }
            } else if (pc.host.equals(host) && (pcName == null || pc.name.equals(pcName))
                    && (pc.protocol != protocol || pc.port != port)) {
                moved = pc;
            }
        }
        if (moved == null) {
            return;
        }
        protocol = moved.protocol;
        pcName = moved.name;
        where = moved.name;
        if (protocol == StreamProtocol.USB) {
            // The PC switches this phone over when it's on the cable.
            usbPcs.add(moved.host + " " + moved.name);
        } else {
            host = moved.host;
            port = moved.port;
            path = moved.path;
        }
        play();
    }

    private void play() {
        stopPlayer();
        showConnecting();
        if (surface != null) {
            startPlayer();
        }
    }

    private void startPlayer() {
        if (player != null) {
            return;
        }
        pictureShown = false;
        player = new NativePlayer(surface, protocol, host, port, path, this);
        if (protocol == StreamProtocol.USB) {
            if (usbWatcher == null) {
                usbWatcher = new UsbAccessoryWatcher(this, this::onUsbStatus);
            }
            usbWatcher.start();
        } else if (!wifiLock.isHeld()) {
            wifiLock.acquire();
        }
    }

    private void stopPlayer() {
        if (player != null) {
            player.release();
            player = null;
        }
        if (usbWatcher != null) {
            usbWatcher.stop();
        }
        if (wifiLock.isHeld()) {
            wifiLock.release();
        }
    }

    @Override
    public void onStatus(int what, @Nullable String detail) {
        switch (what) {
            case NativePlayer.STATUS_PLAYING:
                if (pictureShown) {
                    status.setVisibility(View.GONE);
                } else {
                    showConnecting();
                }
                break;
            case NativePlayer.STATUS_NO_FRAMES:
                showStatus(protocol == StreamProtocol.UDP
                        ? getString(R.string.player_no_frames_udp, host, port)
                        : getString(R.string.player_no_frames, where), null, false);
                break;
            case NativePlayer.STATUS_FAILED:
                showStatus(getString(R.string.player_error, where), detail, true);
                break;
            case NativePlayer.STATUS_RTSP_UDP_BLOCKED:
                showStatus(getString(R.string.player_error, where),
                        getString(R.string.player_rtsp_udp_blocked), true);
                break;
            case NativePlayer.STATUS_USB_CLOSED:
                pictureShown = false;
                showStatus(getString(R.string.player_usb_waiting), null, false);
                break;
            default:
                showConnecting();
                break;
        }
    }

    @Override
    public void onVideoSize(int width, int height) {
        frameLayout.setAspectRatio(height == 0 ? 0 : (float) width / height);
    }

    @Override
    public void onFirstFrame() {
        pictureShown = true;
        status.setVisibility(View.GONE);
    }

    /** USB: what to show while waiting for the PC on the cable; null once it's connected. */
    private void onUsbStatus(@Nullable String text) {
        if (text != null) {
            showStatus(text, null, false);
        } else if (!pictureShown) {
            showConnecting();
        }
    }

    private void showConnecting() {
        showStatus(protocol == StreamProtocol.USB && !NativePlayer.usbOpen()
                ? getString(R.string.player_usb_waiting)
                : getString(R.string.player_connecting, where), null, false);
    }

    private void showStatus(String text, @Nullable String detail, boolean failed) {
        status.setVisibility(View.VISIBLE);
        progress.setVisibility(failed ? View.GONE : View.VISIBLE);
        retry.setVisibility(failed ? View.VISIBLE : View.GONE);
        statusText.setText(text);
        statusDetail.setText(detail);
        statusDetail.setVisibility(detail == null ? View.GONE : View.VISIBLE);
    }
}
