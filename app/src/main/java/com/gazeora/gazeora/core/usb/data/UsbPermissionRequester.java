// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.core.usb.data;

import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.hardware.usb.UsbAccessory;
import android.hardware.usb.UsbManager;

import androidx.annotation.NonNull;

import javax.inject.Inject;

import dagger.hilt.android.qualifiers.ApplicationContext;

/** Asks the user to let the app use the PC's accessory. */
public final class UsbPermissionRequester {
    private static final String ACTION_PERMISSION = "com.gazeora.gazeora.USB_PERMISSION";
    private static final int REQUEST_CODE = 0;

    private final Context context;
    private final UsbManager usb;

    @Inject
    public UsbPermissionRequester(@NonNull @ApplicationContext Context context,
            @NonNull UsbManager usb) {
        this.context = context;
        this.usb = usb;
    }

    public boolean hasPermission(@NonNull UsbAccessory accessory) {
        return usb.hasPermission(accessory);
    }

    public void request(@NonNull UsbAccessory accessory) {
        Intent intent = new Intent(ACTION_PERMISSION).setPackage(context.getPackageName());
        usb.requestPermission(accessory, PendingIntent.getBroadcast(
                context, REQUEST_CODE, intent, PendingIntent.FLAG_MUTABLE));
    }
}
