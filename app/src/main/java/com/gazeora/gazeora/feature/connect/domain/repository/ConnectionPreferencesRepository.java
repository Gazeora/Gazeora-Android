// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.connect.domain.repository;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.core.stream.domain.model.StreamProtocol;

import java.util.Optional;

/** The last manual connection, remembered for next time. */
public interface ConnectionPreferencesRepository {
    @NonNull
    Optional<String> getAddress();

    void saveAddress(@NonNull String address);

    @NonNull
    Optional<StreamProtocol> getProtocol();

    void saveProtocol(@NonNull StreamProtocol protocol);
}
