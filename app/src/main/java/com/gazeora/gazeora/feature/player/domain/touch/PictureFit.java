package com.gazeora.gazeora.feature.player.domain.touch;

/**
 * Where a picture sits in a view it's fitted into whole, at its own shape, with bars on the
 * sides or at the top and bottom; turns spots in the view into spots in the picture.
 */
public final class PictureFit {
    private static final float CENTER = 0.5f;

    private final float left;
    private final float top;
    private final float width;
    private final float height;

    public PictureFit(float viewWidth, float viewHeight, float pictureAspect) {
        float viewAspect = viewHeight <= 0 ? 0 : viewWidth / viewHeight;
        boolean barsOnTheSides = pictureAspect > 0 && pictureAspect < viewAspect;
        boolean barsAtTopAndBottom = pictureAspect > viewAspect && viewAspect > 0;
        this.width = barsOnTheSides ? viewHeight * pictureAspect : viewWidth;
        this.height = barsAtTopAndBottom ? viewWidth / pictureAspect : viewHeight;
        this.left = (viewWidth - width) * CENTER;
        this.top = (viewHeight - height) * CENTER;
    }

    /** 0..1 across the picture; a spot on a bar is at the picture's nearest edge. */
    public float toPictureX(float viewX) {
        return clamp(width <= 0 ? 0 : (viewX - left) / width);
    }

    public float toPictureY(float viewY) {
        return clamp(height <= 0 ? 0 : (viewY - top) / height);
    }

    private static float clamp(float fraction) {
        return Math.max(0f, Math.min(1f, fraction));
    }
}
