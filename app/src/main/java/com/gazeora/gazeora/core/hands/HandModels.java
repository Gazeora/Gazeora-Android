package com.gazeora.gazeora.core.hands;

import android.content.Context;
import android.content.res.AssetManager;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.core.nativebridge.NativeLibrary;

/**
 * The hand tracker's models (src/main/cpp/hands/hand_models.h), loaded as the app starts, so
 * the viewer's hands are ready by the time Gazeora's window shows in a VR viewer.
 */
public final class HandModels {
    static {
        NativeLibrary.load();
    }

    private HandModels() {
    }

    /** Starts loading them in the background; once. */
    public static void preload(@NonNull Context context) {
        Context app = context.getApplicationContext();
        nativePreload(app.getAssets(), app.getCodeCacheDir().getAbsolutePath());
    }

    /** Whether they're loaded, or can't be. */
    public static boolean isReady() {
        return nativeIsReady();
    }

    private static native void nativePreload(@NonNull AssetManager assets, @NonNull String cacheDir);

    private static native boolean nativeIsReady();
}
