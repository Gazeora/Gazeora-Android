// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.core.nativebridge;

/** Loads the native client (src/main/cpp) that the JNI classes call into. */
public final class NativeLibrary {
    private static final String NAME = "gazeora_client";

    private NativeLibrary() {
    }

    /** Safe to call more than once: the system loads it only the first time. */
    public static void load() {
        System.loadLibrary(NAME);
    }
}
