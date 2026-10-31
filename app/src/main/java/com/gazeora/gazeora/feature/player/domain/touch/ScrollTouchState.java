// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.domain.touch;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.feature.player.domain.model.PointerCommand;

/**
 * Fingers moving on the picture: they scroll the way a touch screen does, the content
 * following them; with several, their center counts. The wheel turns in whole notches, as
 * many programs ignore less; what doesn't add up to one yet carries over to the next move.
 */
final class ScrollTouchState implements TouchState {
    // Windows' wheel units in one notch (WHEEL_DELTA).
    private static final int WHEEL_NOTCH = 120;
    // A swipe the picture's height turns the wheel 20 notches.
    private static final float WHEEL_UNITS_PER_PICTURE = 20f * WHEEL_NOTCH;

    private final float anchorX;
    private final float anchorY;
    private final float carryVertical;
    private final float carryHorizontal;

    private ScrollTouchState(float anchorX, float anchorY, float carryVertical,
            float carryHorizontal) {
        this.anchorX = anchorX;
        this.anchorY = anchorY;
        this.carryVertical = carryVertical;
        this.carryHorizontal = carryHorizontal;
    }

    /** The wheel turns whatever is under the PC's pointer: it goes where the fingers are. */
    @NonNull
    static ScrollTouchState startingAt(float x, float y, @NonNull RemotePointer pointer) {
        pointer.send(PointerCommand.move(x, y));
        return new ScrollTouchState(x, y, 0f, 0f);
    }

    @NonNull
    @Override
    public TouchState on(@NonNull TouchSample sample, @NonNull RemotePointer pointer) {
        switch (sample.getPhase()) {
            case MOVE:
                return scroll(sample, pointer);
            case EXTRA_DOWN:
            case EXTRA_UP:
                // The fingers' center jumps; it isn't a move.
                return new ScrollTouchState(sample.getCenterX(), sample.getCenterY(),
                        carryVertical, carryHorizontal);
            default:
                return cancel(pointer);
        }
    }

    @NonNull
    @Override
    public TouchState cancel(@NonNull RemotePointer pointer) {
        return IdleTouchState.INSTANCE;
    }

    @NonNull
    private TouchState scroll(@NonNull TouchSample sample, @NonNull RemotePointer pointer) {
        // Fingers moving up pull the content up: the wheel turns down.
        float vertical = carryVertical
                + (sample.getCenterY() - anchorY) * WHEEL_UNITS_PER_PICTURE;
        float horizontal = carryHorizontal
                - (sample.getCenterX() - anchorX) * WHEEL_UNITS_PER_PICTURE;
        int notchesVertical = wholeNotches(vertical);
        int notchesHorizontal = wholeNotches(horizontal);
        if (notchesVertical != 0 || notchesHorizontal != 0) {
            pointer.send(PointerCommand.scroll(notchesVertical, notchesHorizontal));
        }
        return new ScrollTouchState(sample.getCenterX(), sample.getCenterY(),
                vertical - notchesVertical, horizontal - notchesHorizontal);
    }

    /** The units of the whole notches in `units`, toward zero. */
    private static int wholeNotches(float units) {
        return (int) (units / WHEEL_NOTCH) * WHEEL_NOTCH;
    }
}
