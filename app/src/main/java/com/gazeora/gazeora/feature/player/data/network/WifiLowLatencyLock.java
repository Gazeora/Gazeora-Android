// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.data.network;

import android.net.wifi.WifiManager;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.feature.player.domain.player.NetworkLatencyLock;

import javax.inject.Inject;

/** Wi-Fi power saving holds packets back and drops some, which costs whole frames. */
public final class WifiLowLatencyLock implements NetworkLatencyLock {
    private static final String TAG = "Gazeora:stream";

    private final WifiManager.WifiLock lock;

    @Inject
    public WifiLowLatencyLock(@NonNull WifiManager wifi) {
        lock = wifi.createWifiLock(WifiManager.WIFI_MODE_FULL_LOW_LATENCY, TAG);
        lock.setReferenceCounted(false);
    }

    @Override
    public void acquire() {
        if (!lock.isHeld()) {
            lock.acquire();
        }
    }

    @Override
    public void release() {
        if (lock.isHeld()) {
            lock.release();
        }
    }
}
