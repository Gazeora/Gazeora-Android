// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.core.usb.domain;

/** Where getting the USB connection to the PC stands. */
public enum UsbStatus {
    CONNECTED,
    WAITING_FOR_PC,
    NEEDS_PERMISSION,
    OPEN_FAILED
}
