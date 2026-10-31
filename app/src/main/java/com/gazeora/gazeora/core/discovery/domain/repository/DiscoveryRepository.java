// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.core.discovery.domain.repository;

import androidx.annotation.NonNull;

/** Finds PCs running Gazeora, so nobody has to type an address. PCs that go quiet drop off. */
public interface DiscoveryRepository {
    /** Starts searching, unless already searching. */
    void start(@NonNull PcsListener listener);

    /** Stops searching; nothing more reaches the listener. */
    void stop();
}
