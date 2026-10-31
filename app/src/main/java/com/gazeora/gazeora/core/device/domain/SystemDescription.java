// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.core.device.domain;

import androidx.annotation.NonNull;

/** The phone and Android version, for a problem report. */
public interface SystemDescription {
    @NonNull
    String describe();
}
