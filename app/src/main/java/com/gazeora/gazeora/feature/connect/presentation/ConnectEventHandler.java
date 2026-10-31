// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.connect.presentation;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.core.stream.domain.model.StreamTarget;

/** What the connect screen does for each {@link ConnectEvent}. */
public interface ConnectEventHandler {
    void openPlayer(@NonNull StreamTarget target);

    void requestNetworkPermission();

    void openWebPage(@NonNull String url);
}
