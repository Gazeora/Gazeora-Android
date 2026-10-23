package com.gazeora.gazeora.feature.player.domain.player;

import com.gazeora.gazeora.feature.player.domain.touch.RemotePointer;

/** A stream playing onto the screen; touches go back to the PC the way it came. */
public interface PlaybackSession extends RemotePointer {
    /** Stops playing; nothing more reaches the listener. */
    void release();
}
