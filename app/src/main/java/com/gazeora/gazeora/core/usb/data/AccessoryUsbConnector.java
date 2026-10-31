// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.core.usb.data;

import android.hardware.usb.UsbAccessory;
import android.hardware.usb.UsbManager;
import android.os.ParcelFileDescriptor;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.core.usb.domain.UsbConnector;
import com.gazeora.gazeora.core.usb.domain.UsbStatus;

import java.util.Optional;

import javax.inject.Inject;

/**
 * Finds the PC's accessory, asks for permission once, opens it and hands it to the native
 * client. Opening the accessory is Java-only; the native client does the rest.
 */
public final class AccessoryUsbConnector implements UsbConnector {
    private final UsbLink link;
    private final UsbAccessoryFinder finder;
    private final UsbPermissionRequester permission;
    private final UsbManager usb;
    private boolean askedPermission;

    @Inject
    public AccessoryUsbConnector(@NonNull UsbLink link, @NonNull UsbAccessoryFinder finder,
            @NonNull UsbPermissionRequester permission, @NonNull UsbManager usb) {
        this.link = link;
        this.finder = finder;
        this.permission = permission;
        this.usb = usb;
    }

    @NonNull
    @Override
    public UsbStatus tryConnect() {
        if (link.isOpen()) {
            return UsbStatus.CONNECTED;
        }
        Optional<UsbAccessory> accessory = finder.find();
        if (!accessory.isPresent()) {
            return UsbStatus.WAITING_FOR_PC;
        }
        if (!permission.hasPermission(accessory.get())) {
            return askPermissionOnce(accessory.get());
        }
        askedPermission = false;
        return open(accessory.get());
    }

    @NonNull
    private UsbStatus askPermissionOnce(@NonNull UsbAccessory accessory) {
        if (!askedPermission) {
            askedPermission = true;
            permission.request(accessory);
        }
        return UsbStatus.NEEDS_PERMISSION;
    }

    @NonNull
    private UsbStatus open(@NonNull UsbAccessory accessory) {
        ParcelFileDescriptor descriptor = usb.openAccessory(accessory);
        if (descriptor == null) {
            return UsbStatus.OPEN_FAILED;
        }
        link.attach(descriptor.detachFd());
        return UsbStatus.CONNECTED;
    }
}
