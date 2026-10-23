package com.gazeora.gazeora.core.usb.data;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.core.usb.domain.UsbConnection;

import javax.inject.Inject;

/** The USB connection, from the native link and the accessories Android sees. */
public final class AndroidUsbConnection implements UsbConnection {
    private final UsbLink link;
    private final UsbAccessoryFinder finder;

    @Inject
    public AndroidUsbConnection(@NonNull UsbLink link, @NonNull UsbAccessoryFinder finder) {
        this.link = link;
        this.finder = finder;
    }

    @Override
    public boolean isOpen() {
        return link.isOpen();
    }

    @Override
    public boolean isAvailable() {
        return link.isOpen() || finder.find().isPresent();
    }
}
