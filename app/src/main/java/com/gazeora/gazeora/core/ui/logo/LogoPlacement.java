// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.core.ui.logo;

import androidx.annotation.DrawableRes;

import com.gazeora.gazeora.R;

/** Where the 256-unit logo art sits in each 108-unit logo drawable, and its ring-free copy. */
public enum LogoPlacement {
    /** The splash: smaller, raised to make room for the wordmark. */
    SPLASH(R.drawable.splash_logo_core, 0.175f, 31.6f, 21.6f),
    /** The home screen: as in the launcher icon. */
    HOME(R.drawable.logo_core, 0.275f, 18.8f, 18.8f);

    @DrawableRes
    private final int core;
    private final float scale;
    private final float x;
    private final float y;

    LogoPlacement(@DrawableRes int core, float scale, float x, float y) {
        this.core = core;
        this.scale = scale;
        this.x = x;
        this.y = y;
    }

    @DrawableRes
    public int getCore() {
        return core;
    }

    public float getScale() {
        return scale;
    }

    public float getX() {
        return x;
    }

    public float getY() {
        return y;
    }
}
