package com.gazeora.gazeora.feature.connect.presentation.components;

import android.os.SystemClock;

import androidx.annotation.NonNull;
import androidx.core.splashscreen.SplashScreen;
import androidx.core.splashscreen.SplashScreenViewProvider;

import com.gazeora.gazeora.core.hands.HandModels;

/**
 * Holds the splash a moment on a fresh launch, and on while the hand tracker's models load,
 * up to a limit (the first time, the GPU takes longer); then fades it out.
 */
public final class SplashController {
    private static final long HOLD_MS = 1200;
    private static final long MOST_HOLD_MS = 4000;
    private static final long FADE_MS = 300;
    private static final float TRANSPARENT = 0f;

    private final SplashScreen splash;

    public SplashController(@NonNull SplashScreen splash) {
        this.splash = splash;
    }

    /** Don't hold when recreated (e.g. on rotation) or when the PC wants the stream at once. */
    public void install(boolean hold) {
        if (hold) {
            long hideAt = SystemClock.uptimeMillis() + HOLD_MS;
            long latestAt = SystemClock.uptimeMillis() + MOST_HOLD_MS;
            splash.setKeepOnScreenCondition(() -> {
                long now = SystemClock.uptimeMillis();
                return now < hideAt || (!HandModels.isReady() && now < latestAt);
            });
        }
        splash.setOnExitAnimationListener(this::fadeOut);
    }

    private void fadeOut(@NonNull SplashScreenViewProvider view) {
        view.getView().animate()
                .alpha(TRANSPARENT)
                .setDuration(FADE_MS)
                .withEndAction(view::remove)
                .start();
    }
}
