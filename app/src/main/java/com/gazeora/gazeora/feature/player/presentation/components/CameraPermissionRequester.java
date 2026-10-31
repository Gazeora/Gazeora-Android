// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.presentation.components;

import android.Manifest;
import android.content.pm.PackageManager;

import androidx.activity.ComponentActivity;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;

/**
 * Asks for the camera, which finds the viewer's hands for the drawn ones over Gazeora's window
 * in a VR viewer; its picture never shows.
 * Asked as the player opens, before the phone goes into the viewer, where a dialog can't be
 * answered. Refused, the window shows without them.
 */
public final class CameraPermissionRequester {
    private final ComponentActivity activity;
    private final ActivityResultLauncher<String> launcher;

    /** Create while the activity is being constructed: results need registering that early. */
    public CameraPermissionRequester(@NonNull ComponentActivity activity) {
        this.activity = activity;
        // Either way, nothing to do here: the camera is tried again by itself.
        launcher = activity.registerForActivityResult(
                new ActivityResultContracts.RequestPermission(), granted -> {
                });
    }

    public void requestIfNeeded() {
        if (ContextCompat.checkSelfPermission(activity, Manifest.permission.CAMERA)
                != PackageManager.PERMISSION_GRANTED) {
            launcher.launch(Manifest.permission.CAMERA);
        }
    }
}
