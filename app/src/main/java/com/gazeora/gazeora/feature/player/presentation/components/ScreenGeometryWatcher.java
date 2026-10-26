package com.gazeora.gazeora.feature.player.presentation.components;

import android.app.Activity;
import android.hardware.display.DisplayManager;
import android.os.Build;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.Surface;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.util.Consumer;

import com.gazeora.gazeora.feature.player.domain.model.ScreenGeometry;

/**
 * Observer pattern: passes on how the screen sits, at start and whenever the display turns.
 * A half turn (landscape to the other landscape) changes no configuration, so the display
 * itself is listened to.
 */
public final class ScreenGeometryWatcher implements DisplayManager.DisplayListener {
    private static final float HALF = 2f;

    private final Activity activity;
    private final Consumer<ScreenGeometry> onChanged;
    @Nullable private final DisplayManager displays;

    public ScreenGeometryWatcher(@NonNull Activity activity,
            @NonNull Consumer<ScreenGeometry> onChanged) {
        this.activity = activity;
        this.onChanged = onChanged;
        this.displays = activity.getSystemService(DisplayManager.class);
    }

    public void start() {
        if (displays != null) {
            displays.registerDisplayListener(this, null);  // on this (the main) thread
        }
        onChanged.accept(read());
    }

    public void stop() {
        if (displays != null) {
            displays.unregisterDisplayListener(this);
        }
    }

    @Override
    public void onDisplayAdded(int displayId) {
    }

    @Override
    public void onDisplayRemoved(int displayId) {
    }

    @Override
    public void onDisplayChanged(int displayId) {
        onChanged.accept(read());
    }

    @NonNull
    private ScreenGeometry read() {
        DisplayMetrics metrics = activity.getResources().getDisplayMetrics();
        Display display = display();
        int rotation = display != null ? display.getRotation() : Surface.ROTATION_0;
        return new ScreenGeometry(rotation, (metrics.xdpi + metrics.ydpi) / HALF);
    }

    @Nullable
    @SuppressWarnings("deprecation")  // Activity.getDisplay is API 30; this app starts at 29
    private Display display() {
        return Build.VERSION.SDK_INT >= Build.VERSION_CODES.R ? activity.getDisplay()
                : activity.getWindowManager().getDefaultDisplay();
    }
}
