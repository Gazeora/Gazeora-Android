// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.core.discovery.data.repository;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.core.device.domain.DeviceInfo;
import com.gazeora.gazeora.core.discovery.data.mapper.PcFieldsMapper;
import com.gazeora.gazeora.core.discovery.data.remote.DiscoverySource;
import com.gazeora.gazeora.core.discovery.domain.repository.DiscoveryRepository;
import com.gazeora.gazeora.core.discovery.domain.repository.PcsListener;

import javax.inject.Inject;

/** Repository pattern: discovery replies from the native search, as domain PCs. */
public final class DiscoveryRepositoryImpl implements DiscoveryRepository {
    private final DiscoverySource source;
    private final PcFieldsMapper mapper;
    private final DeviceInfo device;

    @Inject
    public DiscoveryRepositoryImpl(@NonNull DiscoverySource source, @NonNull PcFieldsMapper mapper,
            @NonNull DeviceInfo device) {
        this.source = source;
        this.mapper = mapper;
        this.device = device;
    }

    @Override
    public void start(@NonNull PcsListener listener) {
        source.start(device.isEmulator(), fields -> listener.onPcsChanged(mapper.map(fields)));
    }

    @Override
    public void stop() {
        source.stop();
    }
}
