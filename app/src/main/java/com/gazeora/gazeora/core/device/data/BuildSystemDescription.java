// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.core.device.data;

import android.os.Build;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.core.device.domain.SystemDescription;

import java.util.Locale;

import javax.inject.Inject;
import javax.inject.Singleton;

/** Adapter pattern: describes the phone from {@link Build}. */
@Singleton
public final class BuildSystemDescription implements SystemDescription {
    private static final String FORMAT = "%s %s, Android %s (API %d), %s";

    @Inject
    public BuildSystemDescription() {
    }

    @NonNull
    @Override
    public String describe() {
        return String.format(Locale.ROOT, FORMAT, Build.MANUFACTURER, Build.MODEL,
                Build.VERSION.RELEASE, Build.VERSION.SDK_INT, primaryAbi());
    }

    @NonNull
    private static String primaryAbi() {
        return Build.SUPPORTED_ABIS.length > 0 ? Build.SUPPORTED_ABIS[0] : "";
    }
}
