// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.core.usb.data;

import com.gazeora.gazeora.core.nativebridge.NativeLibrary;

import javax.inject.Inject;
import javax.inject.Singleton;

/** Adapter pattern: the native client's USB link (src/main/cpp: sources/usb_link.h). */
@Singleton
public final class NativeUsbLink implements UsbLink {
    static {
        NativeLibrary.load();
    }

    @Inject
    public NativeUsbLink() {
    }

    @Override
    public boolean isOpen() {
        return nativeIsOpen();
    }

    @Override
    public void attach(int fileDescriptor) {
        nativeAttach(fileDescriptor);
    }

    private static native boolean nativeIsOpen();

    private static native void nativeAttach(int fileDescriptor);
}
