// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.connect.domain.usecase;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.core.stream.domain.model.StreamProtocol;
import com.gazeora.gazeora.feature.connect.domain.repository.ConnectionPreferencesRepository;

import javax.inject.Inject;

/** The protocol to offer: the last one used, or Gazeora's default. */
public final class GetInitialProtocolUseCase {
    private static final StreamProtocol DEFAULT_PROTOCOL = StreamProtocol.RTSP_TCP;

    private final ConnectionPreferencesRepository prefs;

    @Inject
    public GetInitialProtocolUseCase(@NonNull ConnectionPreferencesRepository prefs) {
        this.prefs = prefs;
    }

    @NonNull
    public StreamProtocol execute() {
        return prefs.getProtocol().orElse(DEFAULT_PROTOCOL);
    }
}
