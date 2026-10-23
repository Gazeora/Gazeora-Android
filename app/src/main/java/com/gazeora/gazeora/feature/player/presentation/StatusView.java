package com.gazeora.gazeora.feature.player.presentation;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.gazeora.gazeora.core.ui.UiText;

/** Where the player tells the user how the stream is going. */
public interface StatusView {
    /** The picture is up: nothing to say. */
    void hide();

    /** Still going: shows the message with a spinner. */
    void showProgress(@NonNull UiText message);

    /** Stopped: shows why, with a retry button. */
    void showFailure(@NonNull UiText message, @Nullable UiText detail);
}
