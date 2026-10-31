// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.connect.presentation;

import androidx.annotation.NonNull;

/** Visitor pattern: a one-off thing the connect screen has to do. */
public abstract class ConnectEvent {
    ConnectEvent() {
    }

    public abstract void dispatch(@NonNull ConnectEventHandler handler);
}
