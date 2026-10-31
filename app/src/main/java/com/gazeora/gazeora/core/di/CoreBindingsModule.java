// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.core.di;

import com.gazeora.gazeora.core.device.data.BuildDeviceInfo;
import com.gazeora.gazeora.core.device.data.BuildSystemDescription;
import com.gazeora.gazeora.core.device.domain.DeviceInfo;
import com.gazeora.gazeora.core.device.domain.SystemDescription;
import com.gazeora.gazeora.core.discovery.data.remote.DiscoverySource;
import com.gazeora.gazeora.core.discovery.data.remote.NativeDiscovery;
import com.gazeora.gazeora.core.discovery.data.repository.DiscoveryRepositoryImpl;
import com.gazeora.gazeora.core.discovery.domain.repository.DiscoveryRepository;
import com.gazeora.gazeora.core.network.data.PermissionLocalNetworkAccess;
import com.gazeora.gazeora.core.network.domain.LocalNetworkAccess;
import com.gazeora.gazeora.core.usb.data.AccessoryUsbConnector;
import com.gazeora.gazeora.core.usb.data.AndroidUsbConnection;
import com.gazeora.gazeora.core.usb.data.NativeUsbLink;
import com.gazeora.gazeora.core.usb.data.PollingUsbWatcher;
import com.gazeora.gazeora.core.usb.data.UsbLink;
import com.gazeora.gazeora.core.usb.domain.UsbConnection;
import com.gazeora.gazeora.core.usb.domain.UsbConnector;
import com.gazeora.gazeora.core.usb.domain.UsbWatcher;

import dagger.Binds;
import dagger.Module;
import dagger.hilt.InstallIn;
import dagger.hilt.components.SingletonComponent;

/** Binds core's interfaces. Searches and watchers are unscoped: each screen runs its own. */
@Module
@InstallIn(SingletonComponent.class)
public interface CoreBindingsModule {
    @Binds
    DeviceInfo bindDeviceInfo(BuildDeviceInfo impl);

    @Binds
    SystemDescription bindSystemDescription(BuildSystemDescription impl);

    @Binds
    LocalNetworkAccess bindLocalNetworkAccess(PermissionLocalNetworkAccess impl);

    @Binds
    DiscoverySource bindDiscoverySource(NativeDiscovery impl);

    @Binds
    DiscoveryRepository bindDiscoveryRepository(DiscoveryRepositoryImpl impl);

    @Binds
    UsbLink bindUsbLink(NativeUsbLink impl);

    @Binds
    UsbConnection bindUsbConnection(AndroidUsbConnection impl);

    @Binds
    UsbConnector bindUsbConnector(AccessoryUsbConnector impl);

    @Binds
    UsbWatcher bindUsbWatcher(PollingUsbWatcher impl);
}
