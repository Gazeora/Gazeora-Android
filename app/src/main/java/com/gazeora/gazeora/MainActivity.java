package com.gazeora.gazeora;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.hardware.usb.UsbManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.SystemClock;
import android.provider.Settings;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.widget.ArrayAdapter;
import android.widget.Filter;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.ContextCompat;
import androidx.core.splashscreen.SplashScreen;

import com.google.android.material.textfield.MaterialAutoCompleteTextView;
import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;

import java.util.Collections;
import java.util.List;
import java.util.Locale;

/**
 * The app's home screen: the PCs streaming with Gazeora, found by themselves (and the PC on
 * the USB cable), or one entered by hand. Opens behind the splash (Theme.Gazeora.Starting),
 * or straight into the stream when the PC switched this phone to USB accessory mode.
 */
public class MainActivity extends AppCompatActivity implements Discovery.Listener {
    private static final long SPLASH_HOLD_MS = 1200;
    private static final long SPLASH_FADE_MS = 300;
    private static final int DEFAULT_PORT = 8554;  // Gazeora's, unless it was taken
    private static final String PREF_ADDRESS = "address";
    private static final String PREF_PROTOCOL = "protocol";
    private static final String PERMISSION_LOCAL_NETWORK =
            "android.permission.ACCESS_LOCAL_NETWORK";

    private final ActivityResultLauncher<String> requestLocalNetwork = registerForActivityResult(
            new ActivityResultContracts.RequestPermission(), this::onLocalNetworkResult);

    private Discovery discovery;
    private UsbManager usb;
    private List<Discovery.Pc> pcs = Collections.emptyList();
    private boolean askedForNetwork;
    private Runnable afterPermission;  // what was waiting on the local network permission

    private LinearLayout pcList;
    private View searching;
    private TextView searchHint;
    private View allowNetwork;
    private MaterialAutoCompleteTextView protocolField;
    private StreamProtocol manualProtocol;
    private TextInputLayout addressLayout;
    private TextInputEditText address;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        SplashScreen splash = SplashScreen.installSplashScreen(this);
        super.onCreate(savedInstanceState);
        boolean forUsb = isAccessoryIntent(getIntent());

        // Hold the splash on a fresh launch only: not when recreated (e.g. on rotation), and
        // not when the phone was just plugged into the PC, which wants the stream at once.
        if (savedInstanceState == null && !forUsb) {
            long hideAt = SystemClock.uptimeMillis() + SPLASH_HOLD_MS;
            splash.setKeepOnScreenCondition(() -> SystemClock.uptimeMillis() < hideAt);
        }
        splash.setOnExitAnimationListener(view -> view.getView().animate()
                .alpha(0f)
                .setDuration(SPLASH_FADE_MS)
                .withEndAction(view::remove)
                .start());

        setContentView(R.layout.activity_main);
        usb = (UsbManager) getSystemService(USB_SERVICE);
        discovery = new Discovery(isEmulator(), this);
        pcList = findViewById(R.id.pc_list);
        searching = findViewById(R.id.searching);
        searchHint = findViewById(R.id.search_hint);
        allowNetwork = findViewById(R.id.allow_network);
        allowNetwork.setOnClickListener(v -> askForNetworkAgain());
        protocolField = findViewById(R.id.protocol);
        addressLayout = findViewById(R.id.address_layout);
        address = findViewById(R.id.address);
        setUpManualEntry(savedInstanceState);

        if (savedInstanceState == null && forUsb) {
            openUsb();
        }
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        if (isAccessoryIntent(intent)) {
            openUsb();
        }
    }

    @Override
    protected void onStart() {
        super.onStart();
        if (hasLocalNetwork()) {
            discovery.start();
        } else if (!askedForNetwork) {
            askedForNetwork = true;
            requestLocalNetwork.launch(PERMISSION_LOCAL_NETWORK);
        }
        showPcs();
    }

    @Override
    protected void onStop() {
        super.onStop();
        discovery.stop();
    }

    @Override
    public void onPcsChanged(List<Discovery.Pc> found) {
        pcs = found;
        showPcs();
    }

    private void showPcs() {
        pcList.removeAllViews();
        LayoutInflater inflater = getLayoutInflater();
        if (NativePlayer.usbOpen() || UsbAccessoryWatcher.findAccessory(usb) != null) {
            addPc(inflater, getString(R.string.connect_usb_title),
                    getString(R.string.connect_usb_detail), v -> openUsb());
        }
        for (Discovery.Pc pc : pcs) {
            String detail = pc.protocol == StreamProtocol.USB
                    ? getString(R.string.connect_pc_usb_detail)
                    : getString(pc.protocol.label) + " · " + pc.host + ":" + pc.port + " · "
                            + pc.codec.toUpperCase(Locale.ROOT).replace("H26", "H.26");
            addPc(inflater, pc.name, detail, v -> open(pc));
        }
        boolean network = hasLocalNetwork();
        boolean empty = pcList.getChildCount() == 0;
        searching.setVisibility(network ? View.VISIBLE : View.GONE);
        searchHint.setVisibility(empty || !network ? View.VISIBLE : View.GONE);
        searchHint.setText(network ? R.string.connect_search_hint : R.string.connect_network_needed);
        allowNetwork.setVisibility(network ? View.GONE : View.VISIBLE);
    }

    private void addPc(LayoutInflater inflater, String name, String detail,
            View.OnClickListener onClick) {
        View card = inflater.inflate(R.layout.item_pc, pcList, false);
        ((TextView) card.findViewById(R.id.pc_name)).setText(name);
        ((TextView) card.findViewById(R.id.pc_detail)).setText(detail);
        card.setOnClickListener(onClick);
        pcList.addView(card);
    }

    private void open(Discovery.Pc pc) {
        if (pc.protocol == StreamProtocol.USB) {
            openUsb();
        } else {
            startActivity(PlayerActivity.intent(this, pc.protocol, pc.host, pc.port, pc.path,
                    pc.name));
        }
    }

    private void openUsb() {
        startActivity(PlayerActivity.intent(this, StreamProtocol.USB, null, 0, null, null));
    }

    private void setUpManualEntry(@Nullable Bundle savedInstanceState) {
        StreamProtocol[] protocols = StreamProtocol.values();
        String[] labels = new String[protocols.length];
        for (int i = 0; i < protocols.length; ++i) {
            labels[i] = getString(protocols[i].label);
        }
        protocolField.setAdapter(new AllItemsAdapter(this, labels));
        StreamProtocol saved = StreamProtocol.fromId(prefs().getString(PREF_PROTOCOL, null));
        selectProtocol(saved != null ? saved : StreamProtocol.RTSP_TCP);
        protocolField.setOnItemClickListener((parent, view, position, id) ->
                selectProtocol(protocols[position]));

        if (savedInstanceState == null) {
            address.setText(prefs().getString(PREF_ADDRESS,
                    isEmulator() ? "10.0.2.2:" + DEFAULT_PORT : ""));
        }
        address.setOnEditorActionListener((v, actionId, event) -> {
            if (actionId == EditorInfo.IME_ACTION_GO) {
                watchManually();
                return true;
            }
            return false;
        });
        address.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                addressLayout.setError(null);
            }

            @Override
            public void afterTextChanged(Editable s) {}
        });
        findViewById(R.id.watch).setOnClickListener(v -> watchManually());
    }

    private void selectProtocol(StreamProtocol protocol) {
        manualProtocol = protocol;
        protocolField.setText(getString(protocol.label), false);
        addressLayout.setVisibility(protocol.needsAddress() ? View.VISIBLE : View.GONE);
    }

    private void watchManually() {
        prefs().edit().putString(PREF_PROTOCOL, manualProtocol.id).apply();
        if (!manualProtocol.needsAddress()) {
            openUsb();
            return;
        }
        String input = address.getText() == null ? "" : address.getText().toString().trim();
        Uri uri = Uri.parse("x://" + input.replaceFirst("^[a-zA-Z]+://", ""));
        String host = uri.getHost();
        if (host == null || host.isEmpty() || host.contains(":")) {
            addressLayout.setError(getString(R.string.connect_address_invalid));
            return;
        }
        int port = uri.getPort() == -1 ? DEFAULT_PORT : uri.getPort();
        prefs().edit().putString(PREF_ADDRESS, input).apply();
        StreamProtocol protocol = manualProtocol;
        Runnable play = () -> startActivity(
                PlayerActivity.intent(this, protocol, host, port, null, null));
        // 127.0.0.1 (adb reverse) doesn't need the local network permission.
        if (host.startsWith("127.") || hasLocalNetwork()) {
            play.run();
        } else {
            afterPermission = play;
            requestLocalNetwork.launch(PERMISSION_LOCAL_NETWORK);
        }
    }

    /**
     * Android 17+ drops traffic to the local network (the PC on Wi-Fi, the emulator's host)
     * without this permission, quietly: connections just never get through.
     */
    private boolean hasLocalNetwork() {
        return hasLocalNetwork(this);
    }

    static boolean hasLocalNetwork(Context context) {
        return Build.VERSION.SDK_INT < 37
                || ContextCompat.checkSelfPermission(context, PERMISSION_LOCAL_NETWORK)
                        == PackageManager.PERMISSION_GRANTED;
    }

    private void onLocalNetworkResult(boolean granted) {
        Runnable waiting = afterPermission;
        afterPermission = null;
        if (granted) {
            discovery.start();
            if (waiting != null) {
                waiting.run();
            }
        } else if (waiting != null) {
            addressLayout.setError(getString(R.string.connect_local_network_denied));
        }
        showPcs();
    }

    private void askForNetworkAgain() {
        if (shouldShowRequestPermissionRationale(PERMISSION_LOCAL_NETWORK)) {
            requestLocalNetwork.launch(PERMISSION_LOCAL_NETWORK);
        } else {
            // Turned down for good: only the app's settings can change it now.
            startActivity(new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS,
                    Uri.fromParts("package", getPackageName(), null)));
        }
    }

    private static boolean isAccessoryIntent(Intent intent) {
        return intent != null
                && UsbManager.ACTION_USB_ACCESSORY_ATTACHED.equals(intent.getAction());
    }

    static boolean isEmulator() {
        return Build.FINGERPRINT.startsWith("generic")
                || Build.HARDWARE.contains("ranchu") || Build.HARDWARE.contains("goldfish")
                || Build.PRODUCT.startsWith("sdk");
    }

    private SharedPreferences prefs() {
        return getSharedPreferences("connection", MODE_PRIVATE);
    }

    /**
     * The protocol choices, all of them whatever the field shows. An ArrayAdapter filters by
     * the field's text, so with a protocol chosen (or the text restored after a rotation) the
     * dropdown would offer only that one.
     */
    private static final class AllItemsAdapter extends ArrayAdapter<String> {
        private final String[] items;
        private final Filter all = new Filter() {
            @Override
            protected FilterResults performFiltering(CharSequence constraint) {
                FilterResults results = new FilterResults();
                results.values = items;
                results.count = items.length;
                return results;
            }

            @Override
            protected void publishResults(CharSequence constraint, FilterResults results) {
                notifyDataSetChanged();
            }
        };

        AllItemsAdapter(Context context, String[] items) {
            super(context, R.layout.item_protocol, items);
            this.items = items;
        }

        @NonNull
        @Override
        public Filter getFilter() {
            return all;
        }
    }
}
