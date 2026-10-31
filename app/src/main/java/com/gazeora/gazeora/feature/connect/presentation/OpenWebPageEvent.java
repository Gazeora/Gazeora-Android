// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.connect.presentation;

import androidx.annotation.NonNull;

/** Open {@code url} in the browser. */
final class OpenWebPageEvent extends ConnectEvent {
    private final String url;

    OpenWebPageEvent(@NonNull String url) {
        this.url = url;
    }

    @Override
    public void dispatch(@NonNull ConnectEventHandler handler) {
        handler.openWebPage(url);
    }
}
