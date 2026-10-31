// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.domain.player;

/** Keeps the network fast while a stream plays over it. Both calls are safe to repeat. */
public interface NetworkLatencyLock {
    void acquire();

    void release();
}
