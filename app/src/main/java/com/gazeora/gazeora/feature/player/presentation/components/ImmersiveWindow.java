// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.presentation.components;

import android.view.Window;
import android.view.WindowManager;

import androidx.annotation.NonNull;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;

/** Every pixel for the picture: over the cutout, under hidden system bars, screen kept on. */
public final class ImmersiveWindow {
    private ImmersiveWindow() {
    }

    public static void apply(@NonNull Window window) {
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        window.getAttributes().layoutInDisplayCutoutMode =
                WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
        WindowCompat.setDecorFitsSystemWindows(window, false);
        WindowInsetsControllerCompat bars =
                WindowCompat.getInsetsController(window, window.getDecorView());
        bars.setSystemBarsBehavior(
                WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
        bars.hide(WindowInsetsCompat.Type.systemBars());
    }
}
