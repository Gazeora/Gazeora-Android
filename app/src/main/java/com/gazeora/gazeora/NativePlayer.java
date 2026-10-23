package com.gazeora.gazeora;

import android.media.MediaFormat;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.view.Surface;

import androidx.annotation.Keep;
import androidx.annotation.Nullable;

/**
 * Plays the PC's stream onto a Surface, over any of Gazeora's protocols. The work is in the
 * native client (src/main/cpp: player.h): networking, the protocols and decoding. This picks
 * the decoders, which only Java can list, and brings what happens back to the main thread.
 */
final class NativePlayer {
    static {
        System.loadLibrary("gazeora_client");
    }

    // What the native player says about how playing goes (stream_source.h: PlayerStatus).
    static final int STATUS_CONNECTING = 0;
    static final int STATUS_PLAYING = 1;
    static final int STATUS_NO_FRAMES = 2;
    static final int STATUS_FAILED = 3;
    static final int STATUS_RTSP_UDP_BLOCKED = 4;
    static final int STATUS_USB_CLOSED = 5;

    interface Listener {
        /** On the main thread; {@code detail} says why for {@link #STATUS_FAILED}. */
        void onStatus(int status, @Nullable String detail);

        /** On the main thread. */
        void onVideoSize(int width, int height);

        /** On the main thread, once the first picture is up. */
        void onFirstFrame();
    }

    private final Handler main = new Handler(Looper.getMainLooper());
    private final Listener listener;
    private long handle;

    /**
     * Starts playing; {@code host} and {@code port} aren't needed for USB, and a null path is
     * the protocol's usual one.
     */
    NativePlayer(Surface surface, StreamProtocol protocol, @Nullable String host, int port,
            @Nullable String path, Listener listener) {
        this.listener = listener;
        String avc = Decoders.forMime(MediaFormat.MIMETYPE_VIDEO_AVC);
        String hevc = Decoders.forMime(MediaFormat.MIMETYPE_VIDEO_HEVC);
        handle = nativeStart(surface, protocol.id, host, port, path, avc, hevc,
                lowLatency(avc, MediaFormat.MIMETYPE_VIDEO_AVC),
                lowLatency(hevc, MediaFormat.MIMETYPE_VIDEO_HEVC));
        if (handle == 0) {
            main.post(() -> listener.onStatus(STATUS_FAILED, null));
        }
    }

    private static boolean lowLatency(@Nullable String decoder, String mime) {
        return decoder != null && Build.VERSION.SDK_INT >= 30
                && Decoders.supportsLowLatency(decoder, mime);
    }

    /** Stops playing; nothing more reaches the listener. */
    void release() {
        if (handle != 0) {
            nativeRelease(handle);
            handle = 0;
        }
        main.removeCallbacksAndMessages(null);
    }

    /** Whether the USB connection to the PC is open (the app keeps one for all players). */
    static boolean usbOpen() {
        return nativeUsbOpen();
    }

    /** Hands the PC's accessory over to the native client, which closes it when it goes. */
    static void attachUsb(int fd) {
        nativeUsbAttach(fd);
    }

    // From the native player's threads.

    @Keep
    private void onNativeStatus(int status, @Nullable String detail) {
        main.post(() -> {
            if (handle != 0) {
                listener.onStatus(status, detail);
            }
        });
    }

    @Keep
    private void onNativeVideoSize(int width, int height) {
        main.post(() -> {
            if (handle != 0) {
                listener.onVideoSize(width, height);
            }
        });
    }

    @Keep
    private void onNativeFirstFrame() {
        main.post(() -> {
            if (handle != 0) {
                listener.onFirstFrame();
            }
        });
    }

    private native long nativeStart(Surface surface, String protocol, @Nullable String host,
            int port, @Nullable String path, @Nullable String avcDecoder,
            @Nullable String hevcDecoder, boolean avcLowLatency, boolean hevcLowLatency);

    private static native void nativeRelease(long handle);

    private static native boolean nativeUsbOpen();

    private static native void nativeUsbAttach(int fd);
}
