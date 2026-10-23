package com.gazeora.gazeora.feature.player.presentation;

import androidx.annotation.NonNull;

/** State pattern: what the player shows over the stream. Each state draws itself. */
public abstract class PlayerUiState {
    PlayerUiState() {
    }

    public abstract void render(@NonNull StatusView view);
}
