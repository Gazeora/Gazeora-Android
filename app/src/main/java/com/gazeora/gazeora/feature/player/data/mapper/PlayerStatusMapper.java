// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.data.mapper;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.feature.player.domain.model.PlayerStatus;

import javax.inject.Inject;

/** Mapper pattern: the native player's status codes to {@link PlayerStatus}. */
public final class PlayerStatusMapper {
    // Indexed by the native code (stream_source.h: PlayerStatus).
    private static final PlayerStatus[] BY_CODE = {
            PlayerStatus.CONNECTING,
            PlayerStatus.PLAYING,
            PlayerStatus.NO_FRAMES,
            PlayerStatus.FAILED,
            PlayerStatus.RTSP_UDP_BLOCKED,
            PlayerStatus.USB_CLOSED,
    };

    @Inject
    public PlayerStatusMapper() {
    }

    /** Unknown codes read as connecting. */
    @NonNull
    public PlayerStatus map(int code) {
        return code >= 0 && code < BY_CODE.length ? BY_CODE[code] : PlayerStatus.CONNECTING;
    }
}
