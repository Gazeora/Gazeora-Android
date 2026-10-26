package com.gazeora.gazeora.feature.player.data.room;

import android.graphics.Bitmap;

import androidx.annotation.NonNull;
import androidx.core.util.Consumer;

/** The room view's 360 scene, for when the camera can't show the room. */
public interface PanoramaSource {
    /**
     * Decodes the scene on a background thread and hands it to {@code onLoaded} on that
     * thread; not at all if it can't be read. The receiver recycles it.
     */
    void load(@NonNull Consumer<Bitmap> onLoaded);
}
