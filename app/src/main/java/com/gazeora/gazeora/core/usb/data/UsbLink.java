// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.core.usb.data;

/** The native client's end of the USB connection. */
public interface UsbLink {
    boolean isOpen();

    /** Hands the opened accessory over; the native client closes it when it goes. */
    void attach(int fileDescriptor);
}
