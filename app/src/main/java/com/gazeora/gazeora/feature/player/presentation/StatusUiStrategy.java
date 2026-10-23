package com.gazeora.gazeora.feature.player.presentation;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

/** Strategy pattern: what to show for one player status. */
interface StatusUiStrategy {
    @NonNull
    PlayerUiState create(@NonNull PlaybackSnapshot snapshot, @Nullable String detail);
}
