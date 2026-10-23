package com.gazeora.gazeora.feature.player.presentation;

import androidx.annotation.NonNull;

/** The picture is up. */
final class PlayingUiState extends PlayerUiState {
    @Override
    public void render(@NonNull StatusView view) {
        view.hide();
    }
}
