// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.connect.presentation;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.R;
import com.gazeora.gazeora.core.discovery.domain.model.Pc;
import com.gazeora.gazeora.core.stream.domain.model.StreamTarget;
import com.gazeora.gazeora.core.ui.UiText;

/** A PC found on the network that streams over USB: it needs plugging in. */
final class UsbPcEntry extends PcEntry {
    private final Pc pc;

    UsbPcEntry(@NonNull Pc pc) {
        this.pc = pc;
    }

    @NonNull
    @Override
    public UiText getTitle() {
        return UiText.of(pc.getName());
    }

    @NonNull
    @Override
    public UiText getDetail() {
        return UiText.of(R.string.connect_pc_usb_detail);
    }

    @NonNull
    @Override
    public StreamTarget getTarget() {
        return pc.toStreamTarget();
    }
}
