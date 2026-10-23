package com.gazeora.gazeora.core.usb.domain;

/** The USB connection to the PC, which the app keeps for all players. */
public interface UsbConnection {
    /** Whether the connection is open. */
    boolean isOpen();

    /** Whether the PC is on the cable: connected, or ready to connect. */
    boolean isAvailable();
}
