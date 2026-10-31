// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.presentation;

import androidx.annotation.NonNull;

/** The picture is up. */
final class PlayingUiState extends PlayerUiState {
    @Override
    public void render(@NonNull StatusView view) {
        view.hide();
    }
}
