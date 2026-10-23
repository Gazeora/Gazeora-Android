package com.gazeora.gazeora.core.ui;

import android.content.res.Resources;

import androidx.annotation.NonNull;

/** Text that is already a string, such as a PC's name. */
final class PlainText extends UiText {
    private final String text;

    PlainText(@NonNull String text) {
        this.text = text;
    }

    @NonNull
    @Override
    public String resolve(@NonNull Resources resources) {
        return text;
    }
}
