// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.connect.presentation;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.core.stream.domain.model.StreamTarget;

/** Start playing {@code target}. */
final class OpenPlayerEvent extends ConnectEvent {
    private final StreamTarget target;

    OpenPlayerEvent(@NonNull StreamTarget target) {
        this.target = target;
    }

    @Override
    public void dispatch(@NonNull ConnectEventHandler handler) {
        handler.openPlayer(target);
    }
}
