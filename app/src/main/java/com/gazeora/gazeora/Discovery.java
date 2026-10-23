package com.gazeora.gazeora;

import android.os.Handler;
import android.os.Looper;

import androidx.annotation.Keep;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * Finds PCs running Gazeora, so nobody has to type an address. The native client asks the
 * network once a second (src/main/cpp: discovery.h), and on the emulator the PC it runs on;
 * every Gazeora that hears it answers with its stream. PCs that stop answering drop off.
 */
final class Discovery {
    static {
        System.loadLibrary("gazeora_client");
    }

    /** A PC that answered: where it streams and how. */
    static final class Pc {
        final String name;
        final String host;
        final StreamProtocol protocol;
        final int port;
        final String path;
        final String codec;

        Pc(String name, String host, StreamProtocol protocol, int port, String path,
                String codec) {
            this.name = name;
            this.host = host;
            this.protocol = protocol;
            this.port = port;
            this.path = path;
            this.codec = codec;
        }
    }

    interface Listener {
        /** On the main thread, whenever the list changes. */
        void onPcsChanged(List<Pc> pcs);
    }

    private final boolean emulator;
    private final Listener listener;
    private final Handler main = new Handler(Looper.getMainLooper());
    private long handle;

    Discovery(boolean emulator, Listener listener) {
        this.emulator = emulator;
        this.listener = listener;
    }

    void start() {
        if (handle == 0) {
            handle = nativeStart(emulator);
        }
    }

    void stop() {
        if (handle != 0) {
            nativeStop(handle);
            handle = 0;
        }
        main.removeCallbacksAndMessages(null);
    }

    /** From the native search's thread: six fields a PC (jni.cpp). */
    @Keep
    private void onNativePcs(String[] fields) {
        List<Pc> pcs = new ArrayList<>();
        for (int at = 0; at + 6 <= fields.length; at += 6) {
            StreamProtocol protocol = StreamProtocol.fromId(fields[at + 2]);
            if (protocol != null) {
                pcs.add(new Pc(fields[at], fields[at + 1], protocol,
                        Integer.parseInt(fields[at + 3]), fields[at + 4], fields[at + 5]));
            }
        }
        List<Pc> found = Collections.unmodifiableList(pcs);
        main.post(() -> {
            if (handle != 0) {
                listener.onPcsChanged(found);
            }
        });
    }

    private native long nativeStart(boolean emulator);

    private static native void nativeStop(long handle);
}
