package com.gazeora.gazeora.core.usb.domain;

/** Where getting the USB connection to the PC stands. */
public enum UsbStatus {
    CONNECTED,
    WAITING_FOR_PC,
    NEEDS_PERMISSION,
    OPEN_FAILED
}
