// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.core.stream.domain.model;

/** Gazeora's stream defaults, shared with the PC. */
public final class StreamDefaults {
    /** Gazeora's port, unless it was taken. */
    public static final int PORT = 8554;
    /** The PC the Android emulator runs on. */
    public static final String EMULATOR_HOST = "10.0.2.2";
    /** Addresses that reach this phone itself (adb reverse). */
    public static final String LOOPBACK_PREFIX = "127.";

    private StreamDefaults() {
    }
}
