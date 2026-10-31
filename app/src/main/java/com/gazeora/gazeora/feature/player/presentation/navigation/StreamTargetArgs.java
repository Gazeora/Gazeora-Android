// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.presentation.navigation;

import androidx.annotation.NonNull;
import androidx.lifecycle.SavedStateHandle;

import com.gazeora.gazeora.core.stream.domain.model.StreamProtocol;
import com.gazeora.gazeora.core.stream.domain.model.StreamTarget;

/** Mapper pattern: a {@link StreamTarget} to and from the player screen's extras. */
public final class StreamTargetArgs {
    static final String KEY_PROTOCOL = "protocol";
    static final String KEY_HOST = "host";
    static final String KEY_PORT = "port";
    static final String KEY_PATH = "path";
    static final String KEY_NAME = "name";
    private static final StreamProtocol FALLBACK_PROTOCOL = StreamProtocol.RTSP_TCP;
    private static final int NO_PORT = 0;

    private StreamTargetArgs() {
    }

    @NonNull
    public static StreamTarget read(@NonNull SavedStateHandle args) {
        String protocolId = args.get(KEY_PROTOCOL);
        Integer port = args.get(KEY_PORT);
        return new StreamTarget(
                StreamProtocol.fromId(protocolId).orElse(FALLBACK_PROTOCOL),
                args.get(KEY_HOST),
                port == null ? NO_PORT : port,
                args.get(KEY_PATH),
                args.get(KEY_NAME));
    }
}
