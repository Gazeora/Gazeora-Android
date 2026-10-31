// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.domain.model;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.Objects;

/**
 * One thing for the PC's mouse to do, at a spot in the picture: 0..1 across and down it from
 * the top left. Wheels are in Windows' units (120 a notch), positive scrolling up and right.
 */
public final class PointerCommand {
    private static final float NO_SPOT = 0f;
    private static final int NO_WHEEL = 0;

    private final PointerAction action;
    private final PointerButton button;
    private final float x;
    private final float y;
    private final int wheelVertical;
    private final int wheelHorizontal;

    private PointerCommand(@NonNull PointerAction action, @NonNull PointerButton button, float x,
            float y, int wheelVertical, int wheelHorizontal) {
        this.action = action;
        this.button = button;
        this.x = x;
        this.y = y;
        this.wheelVertical = wheelVertical;
        this.wheelHorizontal = wheelHorizontal;
    }

    @NonNull
    public static PointerCommand move(float x, float y) {
        return new PointerCommand(PointerAction.MOVE, PointerButton.LEFT, x, y, NO_WHEEL,
                NO_WHEEL);
    }

    @NonNull
    public static PointerCommand press(@NonNull PointerButton button, float x, float y) {
        return new PointerCommand(PointerAction.PRESS, button, x, y, NO_WHEEL, NO_WHEEL);
    }

    @NonNull
    public static PointerCommand release(@NonNull PointerButton button) {
        return new PointerCommand(PointerAction.RELEASE, button, NO_SPOT, NO_SPOT, NO_WHEEL,
                NO_WHEEL);
    }

    @NonNull
    public static PointerCommand click(@NonNull PointerButton button, float x, float y) {
        return new PointerCommand(PointerAction.CLICK, button, x, y, NO_WHEEL, NO_WHEEL);
    }

    @NonNull
    public static PointerCommand scroll(int vertical, int horizontal) {
        return new PointerCommand(PointerAction.SCROLL, PointerButton.LEFT, NO_SPOT, NO_SPOT,
                vertical, horizontal);
    }

    @NonNull
    public PointerAction getAction() {
        return action;
    }

    @NonNull
    public PointerButton getButton() {
        return button;
    }

    public float getX() {
        return x;
    }

    public float getY() {
        return y;
    }

    public int getWheelVertical() {
        return wheelVertical;
    }

    public int getWheelHorizontal() {
        return wheelHorizontal;
    }

    @Override
    public boolean equals(@Nullable Object other) {
        if (this == other) {
            return true;
        }
        if (!(other instanceof PointerCommand)) {
            return false;
        }
        PointerCommand that = (PointerCommand) other;
        return action == that.action && button == that.button
                && Float.compare(x, that.x) == 0 && Float.compare(y, that.y) == 0
                && wheelVertical == that.wheelVertical && wheelHorizontal == that.wheelHorizontal;
    }

    @Override
    public int hashCode() {
        return Objects.hash(action, button, x, y, wheelVertical, wheelHorizontal);
    }

    @NonNull
    @Override
    public String toString() {
        return action + " " + button + " at " + x + "," + y + " wheels " + wheelVertical + ","
                + wheelHorizontal;
    }
}
