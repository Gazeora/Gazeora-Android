package com.gazeora.gazeora.feature.player.domain.model;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.Objects;

/** How the stream's picture is shown: for a VR viewer or flat, and its size in pixels. */
public final class StreamPicture {
    /** Before the first picture. */
    public static final StreamPicture NONE = new StreamPicture(false, 0, 0);

    private final boolean vr;
    private final int width;
    private final int height;

    public StreamPicture(boolean vr, int width, int height) {
        this.vr = vr;
        this.width = width;
        this.height = height;
    }

    /** Touches only land somewhere on a flat picture: a VR viewer shows it in two lenses. */
    public boolean acceptsTouch() {
        return !vr && width > 0 && height > 0;
    }

    /** Width over height; only meaningful when it {@link #acceptsTouch()}. */
    public float getAspectRatio() {
        return height == 0 ? 0f : (float) width / height;
    }

    @Override
    public boolean equals(@Nullable Object other) {
        if (this == other) {
            return true;
        }
        if (!(other instanceof StreamPicture)) {
            return false;
        }
        StreamPicture that = (StreamPicture) other;
        return vr == that.vr && width == that.width && height == that.height;
    }

    @Override
    public int hashCode() {
        return Objects.hash(vr, width, height);
    }

    @NonNull
    @Override
    public String toString() {
        return width + "x" + height + (vr ? " VR" : " flat");
    }
}
