package com.gazeora.gazeora.core.discovery.data.remote;

import android.os.Handler;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.gazeora.gazeora.core.nativebridge.NativeLibrary;

import javax.inject.Inject;

/**
 * Adapter pattern: the native search (src/main/cpp: discovery.h) behind {@link DiscoverySource}.
 * It asks the network once a second, and on the emulator the PC it runs on.
 */
public final class NativeDiscovery implements DiscoverySource {
    private static final long NO_HANDLE = 0;

    static {
        NativeLibrary.load();
    }

    private final Handler main;
    @Nullable private PcFieldsListener listener;
    private long handle = NO_HANDLE;

    @Inject
    public NativeDiscovery(@NonNull Handler main) {
        this.main = main;
    }

    @Override
    public void start(boolean emulator, @NonNull PcFieldsListener listener) {
        if (handle == NO_HANDLE) {
            this.listener = listener;
            handle = nativeStart(emulator);
        }
    }

    @Override
    public void stop() {
        if (handle != NO_HANDLE) {
            nativeStop(handle);
            handle = NO_HANDLE;
        }
        listener = null;
        main.removeCallbacksAndMessages(null);
    }

    /** From the native search's thread (jni.cpp). */
    @Keep
    private void onNativePcs(@NonNull String[] fields) {
        main.post(() -> {
            if (handle != NO_HANDLE && listener != null) {
                listener.onPcFields(fields);
            }
        });
    }

    private native long nativeStart(boolean emulator);

    private static native void nativeStop(long handle);
}
