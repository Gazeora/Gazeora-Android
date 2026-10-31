// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.domain.model;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.core.discovery.domain.model.Pc;

import java.util.HashSet;
import java.util.Set;

/** The PCs that said they stream over USB, so the player can follow them off it. */
public final class KnownUsbPcs {
    private final Set<String> keys = new HashSet<>();

    public void remember(@NonNull Pc pc) {
        keys.add(pc.getKey());
    }

    public boolean contains(@NonNull Pc pc) {
        return keys.contains(pc.getKey());
    }
}
