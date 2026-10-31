// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.core.network.data;

import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;

import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;

import com.gazeora.gazeora.core.network.LocalNetworkPermission;
import com.gazeora.gazeora.core.network.domain.LocalNetworkAccess;

import javax.inject.Inject;
import javax.inject.Singleton;

import dagger.hilt.android.qualifiers.ApplicationContext;

/** Checks the local network permission, which older Android versions don't have. */
@Singleton
public final class PermissionLocalNetworkAccess implements LocalNetworkAccess {
    private final Context context;

    @Inject
    public PermissionLocalNetworkAccess(@NonNull @ApplicationContext Context context) {
        this.context = context;
    }

    @Override
    public boolean isGranted() {
        return Build.VERSION.SDK_INT < LocalNetworkPermission.FIRST_SDK
                || ContextCompat.checkSelfPermission(context, LocalNetworkPermission.NAME)
                        == PackageManager.PERMISSION_GRANTED;
    }
}
