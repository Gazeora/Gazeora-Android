// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.domain.touch;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.feature.player.domain.model.PointerButton;
import com.gazeora.gazeora.feature.player.domain.model.PointerCommand;

/**
 * A finger down, not yet moved: a tap if it lifts there, a scroll once it moves or another
 * finger joins.
 */
final class PendingTouchState implements TouchState {
    // How far the finger moves, as a share of the picture, before it's a scroll.
    private static final float SCROLL_DISTANCE = 0.01f;

    private final float downX;
    private final float downY;

    PendingTouchState(float downX, float downY) {
        this.downX = downX;
        this.downY = downY;
    }

    @NonNull
    @Override
    public TouchState on(@NonNull TouchSample sample, @NonNull RemotePointer pointer) {
        switch (sample.getPhase()) {
            case MOVE:
                return moved(sample)
                        ? ScrollTouchState.startingAt(downX, downY, pointer).on(sample, pointer)
                        : this;
            case EXTRA_DOWN:
                return ScrollTouchState.startingAt(sample.getCenterX(), sample.getCenterY(),
                        pointer);
            case LAST_UP:
                pointer.send(PointerCommand.click(PointerButton.LEFT, downX, downY));
                return IdleTouchState.INSTANCE;
            default:
                return cancel(pointer);
        }
    }

    @NonNull
    @Override
    public TouchState cancel(@NonNull RemotePointer pointer) {
        return IdleTouchState.INSTANCE;
    }

    private boolean moved(@NonNull TouchSample sample) {
        return Math.abs(sample.getX() - downX) > SCROLL_DISTANCE
                || Math.abs(sample.getY() - downY) > SCROLL_DISTANCE;
    }
}
