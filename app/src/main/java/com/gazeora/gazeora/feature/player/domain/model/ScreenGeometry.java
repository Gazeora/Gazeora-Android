package com.gazeora.gazeora.feature.player.domain.model;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.Objects;

/**
 * How the phone's screen sits, for the room view: how far the display is turned from the
 * phone's natural orientation, and how dense its pixels are, which says how big the screen is.
 */
public final class ScreenGeometry {
    private static final int QUARTER_TURNS = 4;

    private final int quarterTurns;
    private final float dotsPerInch;

    /** {@code quarterTurns} as Surface.ROTATION_* (0..3); {@code dotsPerInch} 0 if unknown. */
    public ScreenGeometry(int quarterTurns, float dotsPerInch) {
        this.quarterTurns = Math.floorMod(quarterTurns, QUARTER_TURNS);
        this.dotsPerInch = Math.max(dotsPerInch, 0f);
    }

    public int getQuarterTurns() {
        return quarterTurns;
    }

    public float getDotsPerInch() {
        return dotsPerInch;
    }

    @Override
    public boolean equals(@Nullable Object other) {
        if (this == other) {
            return true;
        }
        if (!(other instanceof ScreenGeometry)) {
            return false;
        }
        ScreenGeometry that = (ScreenGeometry) other;
        return quarterTurns == that.quarterTurns
                && Float.compare(dotsPerInch, that.dotsPerInch) == 0;
    }

    @Override
    public int hashCode() {
        return Objects.hash(quarterTurns, dotsPerInch);
    }

    @NonNull
    @Override
    public String toString() {
        return "turned " + quarterTurns + "/4, " + dotsPerInch + " dpi";
    }
}
