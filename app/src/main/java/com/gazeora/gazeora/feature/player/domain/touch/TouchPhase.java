// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.domain.touch;

/** Where a touch on the picture is at. */
public enum TouchPhase {
    /** The first finger went down. */
    FIRST_DOWN,
    MOVE,
    /** Another finger went down while some were already. */
    EXTRA_DOWN,
    /** A finger went up while others stay. */
    EXTRA_UP,
    /** The last finger went up. */
    LAST_UP,
    /** The system took the touch away. */
    CANCEL
}
