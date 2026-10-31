// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.connect.domain.usecase;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.core.network.domain.LocalNetworkAccess;
import com.gazeora.gazeora.core.stream.domain.model.StreamDefaults;
import com.gazeora.gazeora.core.stream.domain.model.StreamTarget;

import javax.inject.Inject;

/** Whether playing {@code target} has to wait for the local network permission. */
public final class NeedsLocalNetworkPermissionUseCase {
    private final LocalNetworkAccess access;

    @Inject
    public NeedsLocalNetworkPermissionUseCase(@NonNull LocalNetworkAccess access) {
        this.access = access;
    }

    public boolean execute(@NonNull StreamTarget target) {
        String host = target.getHost();
        boolean loopback = host != null && host.startsWith(StreamDefaults.LOOPBACK_PREFIX);
        return !loopback && !access.isGranted();
    }
}
