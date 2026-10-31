// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.core.network;

/**
 * Android 17+ quietly drops traffic to the local network without this permission:
 * connections just never get through.
 */
public final class LocalNetworkPermission {
    public static final String NAME = "android.permission.ACCESS_LOCAL_NETWORK";
    public static final int FIRST_SDK = 37;

    private LocalNetworkPermission() {
    }
}
