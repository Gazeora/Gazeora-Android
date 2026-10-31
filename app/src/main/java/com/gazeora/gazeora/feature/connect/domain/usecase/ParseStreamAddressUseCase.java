// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.connect.domain.usecase;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.core.stream.domain.model.StreamDefaults;
import com.gazeora.gazeora.core.stream.domain.model.StreamProtocol;
import com.gazeora.gazeora.core.stream.domain.model.StreamTarget;
import com.gazeora.gazeora.feature.connect.domain.parser.AddressParser;

import java.util.Optional;

import javax.inject.Inject;

/** Turns a typed-in address into what to play; empty if it isn't an address. */
public final class ParseStreamAddressUseCase {
    private final AddressParser parser;

    @Inject
    public ParseStreamAddressUseCase(@NonNull AddressParser parser) {
        this.parser = parser;
    }

    @NonNull
    public Optional<StreamTarget> execute(@NonNull StreamProtocol protocol,
            @NonNull String input) {
        return parser.parse(input.trim()).map(address -> new StreamTarget(protocol,
                address.getHost(), address.getPortOr(StreamDefaults.PORT), null, null));
    }
}
