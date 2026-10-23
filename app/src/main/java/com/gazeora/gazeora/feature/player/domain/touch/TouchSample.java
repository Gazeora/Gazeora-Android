package com.gazeora.gazeora.feature.player.domain.touch;

import androidx.annotation.NonNull;

/**
 * One moment of a touch on the picture, in picture coordinates (0..1 across and down from the
 * top left): the first finger, and the center of the fingers still down.
 */
public final class TouchSample {
    private final TouchPhase phase;
    private final float x;
    private final float y;
    private final float centerX;
    private final float centerY;

    public TouchSample(@NonNull TouchPhase phase, float x, float y, float centerX,
            float centerY) {
        this.phase = phase;
        this.x = x;
        this.y = y;
        this.centerX = centerX;
        this.centerY = centerY;
    }

    @NonNull
    public TouchPhase getPhase() {
        return phase;
    }

    public float getX() {
        return x;
    }

    public float getY() {
        return y;
    }

    public float getCenterX() {
        return centerX;
    }

    public float getCenterY() {
        return centerY;
    }
}
