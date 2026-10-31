// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.domain.touch;

import androidx.annotation.NonNull;

/** No finger down. */
final class IdleTouchState implements TouchState {
    static final IdleTouchState INSTANCE = new IdleTouchState();

    private IdleTouchState() {
    }

    @NonNull
    @Override
    public TouchState on(@NonNull TouchSample sample, @NonNull RemotePointer pointer) {
        if (sample.getPhase() != TouchPhase.FIRST_DOWN) {
            return this;  // the rest of a touch that started while touches were off
        }
        return new PendingTouchState(sample.getX(), sample.getY());
    }

    @NonNull
    @Override
    public TouchState cancel(@NonNull RemotePointer pointer) {
        return this;
    }
}
