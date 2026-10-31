// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.connect.presentation;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.gazeora.gazeora.core.stream.domain.model.StreamProtocol;
import com.gazeora.gazeora.core.ui.UiText;

/** The "connect manually" form: the chosen protocol, and what's wrong with the address. */
public final class ManualEntryUiState {
    private final StreamProtocol protocol;
    @Nullable private final UiText addressError;

    public ManualEntryUiState(@NonNull StreamProtocol protocol, @Nullable UiText addressError) {
        this.protocol = protocol;
        this.addressError = addressError;
    }

    @NonNull
    public StreamProtocol getProtocol() {
        return protocol;
    }

    public boolean showsAddress() {
        return protocol.needsAddress();
    }

    @Nullable
    public UiText getAddressError() {
        return addressError;
    }

    @NonNull
    public ManualEntryUiState withProtocol(@NonNull StreamProtocol newProtocol) {
        return new ManualEntryUiState(newProtocol, addressError);
    }

    @NonNull
    public ManualEntryUiState withAddressError(@Nullable UiText error) {
        return new ManualEntryUiState(protocol, error);
    }
}
