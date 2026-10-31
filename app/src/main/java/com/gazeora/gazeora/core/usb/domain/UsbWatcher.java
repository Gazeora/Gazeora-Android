// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.core.usb.domain;

import androidx.annotation.NonNull;

/** Keeps connecting to the PC over USB, and looks again when the cable comes back. */
public interface UsbWatcher {
    /** Starts watching, unless already watching. Reports only changes, starting from connected. */
    void start(@NonNull UsbStatusListener listener);

    void stop();
}
