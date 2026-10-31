// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.connect.presentation;

import androidx.annotation.NonNull;

/** Ask for the local network permission. */
final class RequestNetworkPermissionEvent extends ConnectEvent {
    @Override
    public void dispatch(@NonNull ConnectEventHandler handler) {
        handler.requestNetworkPermission();
    }
}
