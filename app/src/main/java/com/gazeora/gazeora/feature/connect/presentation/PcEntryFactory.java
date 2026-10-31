// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.connect.presentation;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.core.discovery.domain.model.Pc;

import javax.inject.Inject;

/** Factory pattern: the right card for each PC. */
public final class PcEntryFactory {
    @Inject
    public PcEntryFactory() {
    }

    @NonNull
    public PcEntry usbCable() {
        return new UsbCableEntry();
    }

    @NonNull
    public PcEntry forPc(@NonNull Pc pc) {
        return pc.getProtocol().needsAddress() ? new NetworkPcEntry(pc) : new UsbPcEntry(pc);
    }
}
