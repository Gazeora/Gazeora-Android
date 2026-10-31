// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.domain.model;

/** A button of the PC's mouse. The ids are the wire's (cpp: pointer_message.h). */
public enum PointerButton {
    LEFT(0),
    RIGHT(1);

    private final int wireId;

    PointerButton(int wireId) {
        this.wireId = wireId;
    }

    public int getWireId() {
        return wireId;
    }
}
