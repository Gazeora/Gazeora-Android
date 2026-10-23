package com.gazeora.gazeora.feature.player.presentation.components;

import android.view.Surface;
import android.view.SurfaceHolder;

import androidx.annotation.NonNull;
import androidx.core.util.Consumer;

/** Passes on when the stream's surface comes and goes. */
public final class SurfaceCallback implements SurfaceHolder.Callback {
    private final Consumer<Surface> onCreated;
    private final Runnable onDestroyed;

    public SurfaceCallback(@NonNull Consumer<Surface> onCreated, @NonNull Runnable onDestroyed) {
        this.onCreated = onCreated;
        this.onDestroyed = onDestroyed;
    }

    @Override
    public void surfaceCreated(@NonNull SurfaceHolder holder) {
        onCreated.accept(holder.getSurface());
    }

    @Override
    public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width,
            int height) {
    }

    @Override
    public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
        onDestroyed.run();
    }
}
