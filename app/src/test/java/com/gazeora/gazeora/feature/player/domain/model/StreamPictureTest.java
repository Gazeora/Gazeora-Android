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
    public void noPictureYetTakesNoTouches() {
        assertFalse(StreamPicture.NONE.acceptsTouch());
    }
}
