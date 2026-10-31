// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.core.discovery.domain.repository;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.core.discovery.domain.model.Pc;

import java.util.List;

/** Hears the PCs found on the network. */
public interface PcsListener {
    /** On the main thread, whenever the list changes. */
    void onPcsChanged(@NonNull List<Pc> pcs);
}
