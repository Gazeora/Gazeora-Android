// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.domain.touch;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

public class PictureFitTest {
    private static final float EPSILON = 1e-4f;

    @Test
    public void aWiderViewHasBarsOnTheSides() {
        // 16:9 in a 2000 x 900 view: 1600 wide, 200 of bar each side.
        PictureFit fit = new PictureFit(2000, 900, 16f / 9f);

        assertEquals(0f, fit.toPictureX(200), EPSILON);
        assertEquals(0.5f, fit.toPictureX(1000), EPSILON);
        assertEquals(1f, fit.toPictureX(1800), EPSILON);
        assertEquals(0.5f, fit.toPictureY(450), EPSILON);
    }

    @Test
    public void aTallerViewHasBarsAtTopAndBottom() {
        // 2:1 in a 1000 x 1000 view: 500 tall, 250 of bar above and below.
        PictureFit fit = new PictureFit(1000, 1000, 2f);

        assertEquals(0f, fit.toPictureY(250), EPSILON);
        assertEquals(1f, fit.toPictureY(750), EPSILON);
        assertEquals(0.25f, fit.toPictureX(250), EPSILON);
    }

    @Test
    public void spotsOnABarAreAtTheNearestEdge() {
        PictureFit fit = new PictureFit(2000, 900, 16f / 9f);

        assertEquals(0f, fit.toPictureX(10), EPSILON);
        assertEquals(1f, fit.toPictureX(1990), EPSILON);
    }

    @Test
    public void aPictureOfTheViewsShapeFillsIt() {
        PictureFit fit = new PictureFit(1600, 900, 16f / 9f);

        assertEquals(0.25f, fit.toPictureX(400), EPSILON);
        assertEquals(0.75f, fit.toPictureY(675), EPSILON);
    }
}
