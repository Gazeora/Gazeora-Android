package com.gazeora.gazeora.core.ui;

import android.text.Editable;
import android.text.TextWatcher;

import androidx.annotation.NonNull;

/** Runs an action whenever the text changes. */
public final class TextEditedWatcher implements TextWatcher {
    private final Runnable onEdited;

    public TextEditedWatcher(@NonNull Runnable onEdited) {
        this.onEdited = onEdited;
    }

    @Override
    public void beforeTextChanged(CharSequence s, int start, int count, int after) {
    }

    @Override
    public void onTextChanged(CharSequence s, int start, int before, int count) {
        onEdited.run();
    }

    @Override
    public void afterTextChanged(Editable s) {
    }
}
