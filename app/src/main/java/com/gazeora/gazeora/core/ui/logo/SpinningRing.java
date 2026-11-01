// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.core.ui.logo;

import android.graphics.Path;

import androidx.annotation.NonNull;

/**
 * A circle in 3D, spun about an axis lying in the screen and drawn flat: the half turned away
 * from the viewer into one path, the half facing them into another.
 * Screen coordinates: x right, y down, z towards the viewer.
 */
public final class SpinningRing {
    private static final int SEGMENTS = 120;
    private static final double FULL_TURN = 2 * Math.PI;

    private final float centerX;
    private final float centerY;
    private final double radius;
    private final double cosTilt;
    private final double sinTilt;
    private final double cosRoll;
    private final double sinRoll;
    private final double axisX;
    private final double axisY;

    /**
     * @param tilt how far the circle leans back from facing the viewer, in radians
     * @param roll then how far it turns clockwise on the screen, in radians
     * @param axis the spin axis' angle above the horizontal, in radians
     */
    public SpinningRing(float centerX, float centerY, double radius,
                        double tilt, double roll, double axis) {
        this.centerX = centerX;
        this.centerY = centerY;
        this.radius = radius;
        cosTilt = Math.cos(tilt);
        sinTilt = Math.sin(tilt);
        cosRoll = Math.cos(roll);
        sinRoll = Math.sin(roll);
        axisX = Math.cos(axis);
        axisY = -Math.sin(axis);
    }

    /** Traces the ring turned by {@code spin} radians into the back and front paths. */
    public void trace(double spin, @NonNull Path back, @NonNull Path front) {
        back.rewind();
        front.rewind();
        double cosSpin = Math.cos(spin);
        double sinSpin = Math.sin(spin);
        double[] previous = pointAt(0, cosSpin, sinSpin);
        Path current = null;
        for (int i = 1; i <= SEGMENTS; i++) {
            double[] next = pointAt(FULL_TURN * i / SEGMENTS, cosSpin, sinSpin);
            Path side = previous[2] + next[2] < 0 ? back : front;
            if (side != current) {
                side.moveTo((float) previous[0], (float) previous[1]);
                current = side;
            }
            side.lineTo((float) next[0], (float) next[1]);
            previous = next;
        }
    }

    /** The point at {@code angle} round the ring, as {x, y, z}. */
    @NonNull
    private double[] pointAt(double angle, double cosSpin, double sinSpin) {
        double flatX = radius * Math.cos(angle);
        double flatY = radius * Math.sin(angle);
        double y = flatY * cosTilt;
        double z = flatY * sinTilt;
        double x = flatX * cosRoll - y * sinRoll;
        y = flatX * sinRoll + y * cosRoll;
        return spun(x, y, z, cosSpin, sinSpin);
    }

    /** Rodrigues' rotation about the in-screen axis (axisX, axisY, 0). */
    @NonNull
    private double[] spun(double x, double y, double z, double cosSpin, double sinSpin) {
        double along = (axisX * x + axisY * y) * (1 - cosSpin);
        double crossX = axisY * z;
        double crossY = -axisX * z;
        double crossZ = axisX * y - axisY * x;
        return new double[] {
                centerX + x * cosSpin + crossX * sinSpin + axisX * along,
                centerY + y * cosSpin + crossY * sinSpin + axisY * along,
                z * cosSpin + crossZ * sinSpin,
        };
    }
}
