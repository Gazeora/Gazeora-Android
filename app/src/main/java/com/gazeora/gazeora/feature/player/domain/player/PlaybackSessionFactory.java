package com.gazeora.gazeora.feature.player.domain.player;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.core.stream.domain.model.StreamTarget;

/**
 * Factory pattern: starts playing {@code target} onto a surface. Generic over the surface so
 * the domain stays free of Android types.
 */
public interface PlaybackSessionFactory<S> {
    @NonNull
    PlaybackSession open(@NonNull S surface, @NonNull StreamTarget target,
            @NonNull PlaybackListener listener);
}
