package com.gazeora.gazeora.feature.player.presentation;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.core.ui.UiText;

/** Connecting, or waiting for the picture or the PC. */
final class ProgressUiState extends PlayerUiState {
    private final UiText message;

    ProgressUiState(@NonNull UiText message) {
        this.message = message;
    }

    @Override
    public void render(@NonNull StatusView view) {
        view.showProgress(message);
    }
}
