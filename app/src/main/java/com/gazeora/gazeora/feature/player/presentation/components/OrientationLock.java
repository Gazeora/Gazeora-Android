package com.gazeora.gazeora.feature.player.presentation.components;

import android.app.Activity;
import android.content.pm.ActivityInfo;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.feature.player.domain.model.StreamPicture;

/**
 * Holds the screen in the landscape it's in while the picture is for a VR viewer: tilting the
 * head would otherwise turn the screen over, swapping the eyes. A flat picture turns with the
 * phone again. Between pictures (reconnecting) it stays as it was.
 */
public final class OrientationLock {
    private final Activity activity;

    public OrientationLock(@NonNull Activity activity) {
        this.activity = activity;
    }

    public void follow(@NonNull StreamPicture picture) {
        if (picture.isForViewer()) {
            set(ActivityInfo.SCREEN_ORIENTATION_LOCKED);
        } else if (picture.acceptsTouch()) {
            set(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);  // as the manifest has it
        }
    }

    private void set(int orientation) {
        if (activity.getRequestedOrientation() != orientation) {
            activity.setRequestedOrientation(orientation);
        }
    }
}
