package com.gazeora.gazeora.core.usb.data;

/** The native client's end of the USB connection. */
public interface UsbLink {
    boolean isOpen();

    /** Hands the opened accessory over; the native client closes it when it goes. */
    void attach(int fileDescriptor);
}
