// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.presentation;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.core.stream.domain.model.StreamTarget;

/** What the player state depends on, at one moment. */
public final class PlaybackSnapshot {
    private final StreamTarget target;
    private final boolean pictureShown;
    private final boolean usbOpen;

    public PlaybackSnapshot(@NonNull StreamTarget target, boolean pictureShown, boolean usbOpen) {
        this.target = target;
        this.pictureShown = pictureShown;
        this.usbOpen = usbOpen;
    }

    @NonNull
    public StreamTarget getTarget() {
        return target;
    }

    public boolean isPictureShown() {
        return pictureShown;
    }

    public boolean isUsbOpen() {
        return usbOpen;
    }
}
