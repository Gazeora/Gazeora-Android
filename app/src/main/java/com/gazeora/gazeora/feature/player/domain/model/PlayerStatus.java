// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.domain.model;

/** How playing goes. */
public enum PlayerStatus {
    CONNECTING,
    /** Frames are arriving (again). */
    PLAYING,
    /** Connected, but nothing has arrived for a while. */
    NO_FRAMES,
    /** Playing has stopped; the detail says why. */
    FAILED,
    /** RTSP over UDP set up, but no video gets through. */
    RTSP_UDP_BLOCKED,
    /** The cable went, or the PC let go: waiting for it again. */
    USB_CLOSED
}
