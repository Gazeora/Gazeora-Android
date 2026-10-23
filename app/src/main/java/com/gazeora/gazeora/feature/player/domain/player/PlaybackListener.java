package com.gazeora.gazeora.feature.player.domain.player;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.gazeora.gazeora.feature.player.domain.model.PlayerStatus;
import com.gazeora.gazeora.feature.player.domain.model.StreamPicture;

/** Hears how playing goes; all on the main thread. */
public interface PlaybackListener {
    /** {@code detail} says why for {@link PlayerStatus#FAILED}. */
    void onStatus(@NonNull PlayerStatus status, @Nullable String detail);

    void onVideoSize(int width, int height);

    /** Once the first picture is up. */
    void onFirstFrame();

    /** When the picture starts showing for a VR viewer or flat, or changes size. */
    void onPicture(@NonNull StreamPicture picture);
}
