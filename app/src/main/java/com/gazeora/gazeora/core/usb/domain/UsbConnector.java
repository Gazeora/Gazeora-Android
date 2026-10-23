package com.gazeora.gazeora.core.usb.domain;

import androidx.annotation.NonNull;

/** Takes one step towards the USB connection to the PC. */
public interface UsbConnector {
    @NonNull
    UsbStatus tryConnect();
}
