// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.data.decoder;

import androidx.annotation.NonNull;

import java.util.Optional;

/** The video decoders on this device, which only Java can list. */
public interface DecoderCatalog {
    /** The decoder to use for {@code mime}, if the device has one. */
    @NonNull
    Optional<String> decoderFor(@NonNull String mime);

    boolean supportsLowLatency(@NonNull String decoder, @NonNull String mime);
}
