package com.gazeora.gazeora.feature.player.domain.player;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.feature.player.domain.model.ScreenGeometry;
import com.gazeora.gazeora.feature.player.domain.touch.RemotePointer;

/** A stream playing onto the screen; touches go back to the PC the way it came. */
public interface PlaybackSession extends RemotePointer {
    /** How the screen sits now, for the room view; again whenever it changes. */
    void setScreen(@NonNull ScreenGeometry screen);

    /** Stops playing; nothing more reaches the listener. */
    void release();
}
