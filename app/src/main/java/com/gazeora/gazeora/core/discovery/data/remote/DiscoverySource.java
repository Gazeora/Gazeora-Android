// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.core.discovery.data.remote;

import androidx.annotation.NonNull;

/** Asks the network for PCs and passes on what they answer. */
public interface DiscoverySource {
    void start(boolean emulator, @NonNull PcFieldsListener listener);

    void stop();
}
