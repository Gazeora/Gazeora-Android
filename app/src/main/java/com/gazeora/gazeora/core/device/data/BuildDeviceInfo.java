// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.core.device.data;

import android.os.Build;

import com.gazeora.gazeora.core.device.domain.DeviceInfo;

import javax.inject.Inject;
import javax.inject.Singleton;

/** Reads the device facts from {@link Build}. */
@Singleton
public final class BuildDeviceInfo implements DeviceInfo {
    private static final String EMULATOR_FINGERPRINT = "generic";
    private static final String EMULATOR_HARDWARE_RANCHU = "ranchu";
    private static final String EMULATOR_HARDWARE_GOLDFISH = "goldfish";
    private static final String EMULATOR_PRODUCT = "sdk";

    @Inject
    public BuildDeviceInfo() {
    }

    @Override
    public boolean isEmulator() {
        return Build.FINGERPRINT.startsWith(EMULATOR_FINGERPRINT)
                || Build.HARDWARE.contains(EMULATOR_HARDWARE_RANCHU)
                || Build.HARDWARE.contains(EMULATOR_HARDWARE_GOLDFISH)
                || Build.PRODUCT.startsWith(EMULATOR_PRODUCT);
    }
}
