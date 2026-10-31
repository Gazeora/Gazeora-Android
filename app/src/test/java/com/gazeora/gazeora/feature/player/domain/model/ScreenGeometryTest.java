// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.domain.model;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;

import org.junit.Test;

public class ScreenGeometryTest {
    private static final float DELTA = 0f;

    @Test
    public void keepsTheTurnWithinAWholeOne() {
        assertEquals(1, new ScreenGeometry(1, 400f).getQuarterTurns());
        assertEquals(3, new ScreenGeometry(-1, 400f).getQuarterTurns());
        assertEquals(0, new ScreenGeometry(4, 400f).getQuarterTurns());
    }

    @Test
    public void anUnknownDensityIsZero() {
        assertEquals(0f, new ScreenGeometry(1, -3f).getDotsPerInch(), DELTA);
    }

    @Test
    public void equalWhenTurnAndDensityAre() {
        assertEquals(new ScreenGeometry(1, 400f), new ScreenGeometry(1, 400f));
        assertEquals(new ScreenGeometry(1, 400f).hashCode(),
                new ScreenGeometry(1, 400f).hashCode());
        assertNotEquals(new ScreenGeometry(3, 400f), new ScreenGeometry(1, 400f));
    }
}
