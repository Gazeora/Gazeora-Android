package com.gazeora.gazeora.core.ui;

import android.content.res.Resources;

import androidx.annotation.NonNull;
import androidx.annotation.StringRes;

/** Text for the screen, resolved only once there are resources, so ViewModels hold no Context. */
public abstract class UiText {
    UiText() {
    }

    @NonNull
    public static UiText of(@NonNull String text) {
        return new PlainText(text);
    }

    /** Arguments that are themselves UiText get resolved too. */
    @NonNull
    public static UiText of(@StringRes int resId, @NonNull Object... args) {
        return new ResourceText(resId, args);
    }

    @NonNull
    public abstract String resolve(@NonNull Resources resources);
}
