// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.domain.touch;

import androidx.annotation.NonNull;

import javax.inject.Inject;

/**
 * Turns touches on the picture into the PC's mouse, like a touch screen: a tap clicks, a
 * finger moving scrolls. Nothing is ever left held down on the PC.
 */
public final class TouchGestureInterpreter {
    private final RemotePointer pointer;
    private TouchState state = IdleTouchState.INSTANCE;

    @Inject
    public TouchGestureInterpreter(@NonNull RemotePointer pointer) {
        this.pointer = pointer;
    }

    public void onTouch(@NonNull TouchSample sample) {
        state = state.on(sample, pointer);
    }

    /** Forgets the touch under way: the next one starts afresh. */
    public void cancel() {
        state = state.cancel(pointer);
    }
}
