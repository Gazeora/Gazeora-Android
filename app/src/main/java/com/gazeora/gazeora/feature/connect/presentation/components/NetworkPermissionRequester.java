// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.connect.presentation.components;

import android.content.Intent;
import android.net.Uri;
import android.provider.Settings;

import androidx.activity.ComponentActivity;
import androidx.activity.result.ActivityResultCallback;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;

import com.gazeora.gazeora.core.network.LocalNetworkPermission;

/** Asks for the local network permission, or sends the user to Settings once it's refused. */
public final class NetworkPermissionRequester {
    private static final String PACKAGE_SCHEME = "package";

    private final ComponentActivity activity;
    private final ActivityResultLauncher<String> launcher;

    /** Create while the activity is being constructed: results need registering that early. */
    public NetworkPermissionRequester(@NonNull ComponentActivity activity,
            @NonNull ActivityResultCallback<Boolean> onResult) {
        this.activity = activity;
        launcher = activity.registerForActivityResult(
                new ActivityResultContracts.RequestPermission(), onResult);
    }

    public void request() {
        launcher.launch(LocalNetworkPermission.NAME);
    }

    public void requestAgain() {
        if (activity.shouldShowRequestPermissionRationale(LocalNetworkPermission.NAME)) {
            request();
        } else {
            // Turned down for good: only the app's settings can change it now.
            activity.startActivity(new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS,
                    Uri.fromParts(PACKAGE_SCHEME, activity.getPackageName(), null)));
        }
    }
}
