// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.domain.model;

/** What a touch asks of the PC's mouse. The ids are the wire's (cpp: pointer_message.h). */
public enum PointerAction {
    MOVE(1),
    PRESS(2),
    RELEASE(3),
    SCROLL(4),
    /** A press and release in one, so a lost message can't leave the button held. */
    CLICK(5);

    private final int wireId;

    PointerAction(int wireId) {
        this.wireId = wireId;
    }

    public int getWireId() {
        return wireId;
    }
}
