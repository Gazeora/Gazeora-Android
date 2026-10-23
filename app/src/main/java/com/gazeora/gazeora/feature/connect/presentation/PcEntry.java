package com.gazeora.gazeora.feature.connect.presentation;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.core.stream.domain.model.StreamTarget;
import com.gazeora.gazeora.core.ui.UiText;

/** A card in the list of PCs: what it says, and what tapping it plays. */
public abstract class PcEntry {
    PcEntry() {
    }

    @NonNull
    public abstract UiText getTitle();

    @NonNull
    public abstract UiText getDetail();

    @NonNull
    public abstract StreamTarget getTarget();
}
