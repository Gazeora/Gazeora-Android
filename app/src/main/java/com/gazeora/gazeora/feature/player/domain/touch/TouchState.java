package com.gazeora.gazeora.feature.player.domain.touch;

import androidx.annotation.NonNull;

/** State pattern: what a touch is doing to the PC's mouse so far. */
interface TouchState {
    /** Sends what `sample` means to `pointer`; the state after it. */
    @NonNull
    TouchState on(@NonNull TouchSample sample, @NonNull RemotePointer pointer);

    /** Forgets the touch under way; the state after is idle. */
    @NonNull
    TouchState cancel(@NonNull RemotePointer pointer);
}
