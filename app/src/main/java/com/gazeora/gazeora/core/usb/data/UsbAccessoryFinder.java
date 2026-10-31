// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.core.usb.data;

import android.hardware.usb.UsbAccessory;
import android.hardware.usb.UsbManager;

import androidx.annotation.NonNull;

import java.util.Optional;

import javax.inject.Inject;

/** Finds the PC once it has switched this phone to USB accessory mode. */
public final class UsbAccessoryFinder {
    // What the PC calls itself; res/xml/accessory_filter.xml matches it too.
    private static final String MANUFACTURER = "Gazeora";
    private static final String MODEL = "Gazeora Stream";

    private final UsbManager usb;

    @Inject
    public UsbAccessoryFinder(@NonNull UsbManager usb) {
        this.usb = usb;
    }

    @NonNull
    public Optional<UsbAccessory> find() {
        UsbAccessory[] accessories = usb.getAccessoryList();
        if (accessories == null) {
            return Optional.empty();
        }
        for (UsbAccessory accessory : accessories) {
            if (MANUFACTURER.equals(accessory.getManufacturer())
                    && MODEL.equals(accessory.getModel())) {
                return Optional.of(accessory);
            }
        }
        return Optional.empty();
    }
}
