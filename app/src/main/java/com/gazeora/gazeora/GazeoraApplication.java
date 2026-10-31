// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora;

import android.app.Application;

import com.gazeora.gazeora.core.hands.HandModels;

import dagger.hilt.android.HiltAndroidApp;

/** Hilt's entry point for the whole app. */
@HiltAndroidApp
public final class GazeoraApplication extends Application {
    @Override
    public void onCreate() {
        super.onCreate();
        HandModels.preload(this);  // behind the splash
    }
}
