// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.domain.model;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class StreamPictureTest {
    @Test
    public void onlyAFlatPictureTakesTouches() {
        assertTrue(new StreamPicture(false, 1920, 1080).acceptsTouch());
        assertFalse(new StreamPicture(true, 1920, 1080).acceptsTouch());
    }

    @Test
    public void knowsWhenItsForAViewer() {
        assertTrue(new StreamPicture(true, 1920, 1080).isForViewer());
        assertFalse(new StreamPicture(false, 1920, 1080).isForViewer());
        assertFalse(StreamPicture.NONE.isForViewer());
    }

    @Test
    public void noPictureYetTakesNoTouches() {
        assertFalse(StreamPicture.NONE.acceptsTouch());
    }
}
