// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.connect.presentation;

import androidx.annotation.NonNull;

import java.util.Locale;

/** Codec ids for people: "h265" reads "H.265". */
final class CodecLabels {
    private static final String CODEC_FAMILY = "H26";
    private static final String CODEC_FAMILY_LABEL = "H.26";

    private CodecLabels() {
    }

    @NonNull
    static String of(@NonNull String codec) {
        return codec.toUpperCase(Locale.ROOT).replace(CODEC_FAMILY, CODEC_FAMILY_LABEL);
    }
}
