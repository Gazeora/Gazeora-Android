package com.gazeora.gazeora.feature.player.presentation;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.gazeora.gazeora.core.ui.UiText;

/** Playing stopped; the user can retry. */
final class FailedUiState extends PlayerUiState {
    private final UiText message;
    @Nullable private final UiText detail;

    FailedUiState(@NonNull UiText message, @Nullable UiText detail) {
        this.message = message;
        this.detail = detail;
    }

    @Override
    public void render(@NonNull StatusView view) {
        view.showFailure(message, detail);
    }
}
