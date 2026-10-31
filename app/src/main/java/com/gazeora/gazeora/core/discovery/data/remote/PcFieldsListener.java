// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.core.discovery.data.remote;

import androidx.annotation.NonNull;

/** Hears the raw discovery replies: six strings a PC. */
public interface PcFieldsListener {
    /** On the main thread. */
    void onPcFields(@NonNull String[] fields);
}
