// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.connect.presentation.components;

import android.os.Build;
import android.os.SystemClock;
import android.view.View;
import android.widget.ImageView;

import androidx.annotation.NonNull;
import androidx.core.splashscreen.SplashScreen;
import androidx.core.splashscreen.SplashScreenViewProvider;

import com.gazeora.gazeora.core.hands.HandModels;
import com.gazeora.gazeora.core.ui.logo.GyroscopeLogoDrawable;
import com.gazeora.gazeora.core.ui.logo.LogoPlacement;

/**
 * Holds the splash a moment on a fresh launch, and on while the hand tracker's models load,
 * up to a limit (the first time, the GPU takes longer); then fades it out.
 */
public final class SplashController {
    private static final long HOLD_MS = 1200;
    private static final long MOST_HOLD_MS = 4000;
    private static final long FADE_MS = 300;
    private static final long RECHECK_MS = 50;
    private static final float TRANSPARENT = 0f;

    private final SplashScreen splash;

    public SplashController(@NonNull SplashScreen splash) {
        this.splash = splash;
    }

    /** Don't hold when recreated (e.g. on rotation) or when the PC wants the stream at once. */
    public void install(boolean hold) {
        SplashScreen.KeepOnScreenCondition keep = hold ? holdCondition() : () -> false;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            // The system spins the rings itself.
            splash.setKeepOnScreenCondition(keep);
            splash.setOnExitAnimationListener(this::fadeOut);
        } else {
            // Below Android 12 the library shows a still icon while it holds, so it's shown
            // at once and held here, with the rings spun in 3D by hand.
            splash.setOnExitAnimationListener(view -> {
                long spinStart = SystemClock.uptimeMillis();
                long cycleMs = spinRings(view)
                        ? GyroscopeLogoDrawable.cycleMs(view.getView().getContext()) : 0;
                fadeOutWhenDone(view, keep, spinStart, cycleMs);
            });
        }
    }

    @NonNull
    private static SplashScreen.KeepOnScreenCondition holdCondition() {
        long hideAt = SystemClock.uptimeMillis() + HOLD_MS;
        long latestAt = SystemClock.uptimeMillis() + MOST_HOLD_MS;
        return () -> {
            long now = SystemClock.uptimeMillis();
            return now < hideAt || (!HandModels.isReady() && now < latestAt);
        };
    }

    /** Returns whether the rings are spinning. */
    private static boolean spinRings(@NonNull SplashScreenViewProvider view) {
        View icon = view.getIconView();
        if (!(icon instanceof ImageView)) {
            return false;
        }
        GyroscopeLogoDrawable logo =
                new GyroscopeLogoDrawable(icon.getContext(), LogoPlacement.SPLASH);
        ((ImageView) icon).setImageDrawable(logo);
        logo.start();
        return true;
    }

    private void fadeOutWhenDone(@NonNull SplashScreenViewProvider view,
                                 @NonNull SplashScreen.KeepOnScreenCondition keep,
                                 long spinStart, long cycleMs) {
        if (keep.shouldKeepOnScreen()) {
            view.getView().postDelayed(
                    () -> fadeOutWhenDone(view, keep, spinStart, cycleMs), RECHECK_MS);
        } else {
            view.getView().postDelayed(() -> fadeOut(view), untilCycleEnds(spinStart, cycleMs));
        }
    }

    /** How long to wait so the fade ends with the rings home, rather than mid-spin. */
    private static long untilCycleEnds(long spinStart, long cycleMs) {
        if (cycleMs <= 0) {
            return 0;
        }
        long spunByFadeEnd = SystemClock.uptimeMillis() - spinStart + FADE_MS;
        return (cycleMs - spunByFadeEnd % cycleMs) % cycleMs;
    }

    private void fadeOut(@NonNull SplashScreenViewProvider view) {
        view.getView().animate()
                .alpha(TRANSPARENT)
                .setDuration(FADE_MS)
                .withEndAction(view::remove)
                .start();
    }
}
