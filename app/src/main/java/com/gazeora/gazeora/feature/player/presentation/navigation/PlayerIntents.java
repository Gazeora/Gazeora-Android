// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.presentation.navigation;

import android.content.Context;
import android.content.Intent;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.core.stream.domain.model.StreamTarget;
import com.gazeora.gazeora.feature.player.presentation.PlayerActivity;

/** The way into the player screen from other features. */
public final class PlayerIntents {
    private PlayerIntents() {
    }

    @NonNull
    public static Intent create(@NonNull Context context, @NonNull StreamTarget target) {
        return new Intent(context, PlayerActivity.class)
                .putExtra(StreamTargetArgs.KEY_PROTOCOL, target.getProtocol().getId())
                .putExtra(StreamTargetArgs.KEY_HOST, target.getHost())
                .putExtra(StreamTargetArgs.KEY_PORT, target.getPort())
                .putExtra(StreamTargetArgs.KEY_PATH, target.getPath())
                .putExtra(StreamTargetArgs.KEY_NAME, target.getName());
    }
}
