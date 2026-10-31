// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.presentation;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

/** Strategy pattern: what to show for one player status. */
interface StatusUiStrategy {
    @NonNull
    PlayerUiState create(@NonNull PlaybackSnapshot snapshot, @Nullable String detail);
}
