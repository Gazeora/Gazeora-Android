// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.connect.presentation;

import androidx.annotation.NonNull;
import androidx.annotation.StringRes;

import com.gazeora.gazeora.R;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/** The PCs to offer, and whether the search can run. */
public final class PcListUiState {
    private final List<PcEntry> entries;
    private final boolean networkAvailable;

    public PcListUiState(@NonNull List<PcEntry> entries, boolean networkAvailable) {
        this.entries = Collections.unmodifiableList(new ArrayList<>(entries));
        this.networkAvailable = networkAvailable;
    }

    @NonNull
    public List<PcEntry> getEntries() {
        return entries;
    }

    public boolean isSearching() {
        return networkAvailable;
    }

    public boolean showsHint() {
        return entries.isEmpty() || !networkAvailable;
    }

    @StringRes
    public int getHint() {
        return networkAvailable ? R.string.connect_search_hint : R.string.connect_network_needed;
    }

    public boolean showsAllowNetwork() {
        return !networkAvailable;
    }
}
