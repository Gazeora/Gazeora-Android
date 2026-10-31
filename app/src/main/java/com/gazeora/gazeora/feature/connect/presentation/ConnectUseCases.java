// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.connect.presentation;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.feature.connect.domain.usecase.GetInitialAddressUseCase;
import com.gazeora.gazeora.feature.connect.domain.usecase.GetInitialProtocolUseCase;
import com.gazeora.gazeora.feature.connect.domain.usecase.NeedsLocalNetworkPermissionUseCase;
import com.gazeora.gazeora.feature.connect.domain.usecase.ParseStreamAddressUseCase;
import com.gazeora.gazeora.feature.connect.domain.usecase.SaveManualConnectionUseCase;

import javax.inject.Inject;

/** Facade pattern: the connect screen's use cases, injected as one. */
public final class ConnectUseCases {
    private final ParseStreamAddressUseCase parseAddress;
    private final NeedsLocalNetworkPermissionUseCase needsPermission;
    private final GetInitialAddressUseCase initialAddress;
    private final GetInitialProtocolUseCase initialProtocol;
    private final SaveManualConnectionUseCase saveConnection;

    @Inject
    public ConnectUseCases(@NonNull ParseStreamAddressUseCase parseAddress,
            @NonNull NeedsLocalNetworkPermissionUseCase needsPermission,
            @NonNull GetInitialAddressUseCase initialAddress,
            @NonNull GetInitialProtocolUseCase initialProtocol,
            @NonNull SaveManualConnectionUseCase saveConnection) {
        this.parseAddress = parseAddress;
        this.needsPermission = needsPermission;
        this.initialAddress = initialAddress;
        this.initialProtocol = initialProtocol;
        this.saveConnection = saveConnection;
    }

    @NonNull
    public ParseStreamAddressUseCase parseAddress() {
        return parseAddress;
    }

    @NonNull
    public NeedsLocalNetworkPermissionUseCase needsPermission() {
        return needsPermission;
    }

    @NonNull
    public GetInitialAddressUseCase initialAddress() {
        return initialAddress;
    }

    @NonNull
    public GetInitialProtocolUseCase initialProtocol() {
        return initialProtocol;
    }

    @NonNull
    public SaveManualConnectionUseCase saveConnection() {
        return saveConnection;
    }
}
