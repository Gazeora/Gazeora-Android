// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.connect.presentation.components;

import android.app.Activity;
import android.widget.TextView;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.R;

/**
 * The home screen's About section: the attribution "Gazeora by Nithin Aashik" with the app's
 * version, which NOTICE's additional terms (GPLv3 section 7(b)) keep in every version.
 */
public final class AboutView {
    private final Activity activity;
    private final TextView attribution;

    public AboutView(@NonNull Activity activity) {
        this.activity = activity;
        this.attribution = activity.findViewById(R.id.about_attribution);
    }

    public void render(@NonNull String version) {
        attribution.setText(activity.getString(R.string.about_attribution, version));
    }
}
