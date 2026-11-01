// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.connect.presentation.components;

import android.app.Activity;
import android.widget.ImageView;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.R;
import com.gazeora.gazeora.core.ui.logo.GyroscopeLogoDrawable;
import com.gazeora.gazeora.core.ui.logo.LogoPlacement;

/** The home screen's logo, its rings spinning as on the splash. */
public final class HomeLogoView {
    private final GyroscopeLogoDrawable logo;

    public HomeLogoView(@NonNull Activity activity) {
        logo = new GyroscopeLogoDrawable(activity, LogoPlacement.HOME);
        ImageView view = activity.findViewById(R.id.home_logo);
        view.setImageDrawable(logo);
    }

    /** It pauses by itself while the screen isn't showing. */
    public void spin() {
        logo.start();
    }
}
