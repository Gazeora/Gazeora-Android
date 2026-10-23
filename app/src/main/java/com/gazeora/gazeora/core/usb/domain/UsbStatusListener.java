package com.gazeora.gazeora.core.usb.domain;

import androidx.annotation.NonNull;

/** Hears changes in the USB connection's status. */
public interface UsbStatusListener {
    /** On the main thread. */
    void onUsbStatus(@NonNull UsbStatus status);
}
