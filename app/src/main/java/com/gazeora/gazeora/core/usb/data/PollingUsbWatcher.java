package com.gazeora.gazeora.core.usb.data;

import android.os.Handler;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.gazeora.gazeora.core.usb.domain.UsbConnector;
import com.gazeora.gazeora.core.usb.domain.UsbStatus;
import com.gazeora.gazeora.core.usb.domain.UsbStatusListener;
import com.gazeora.gazeora.core.usb.domain.UsbWatcher;

import javax.inject.Inject;

/** Looks twice a second, so it also reconnects when the cable comes back. */
public final class PollingUsbWatcher implements UsbWatcher {
    private static final long LOOK_EVERY_MS = 500;

    private final UsbConnector connector;
    private final Handler main;
    @Nullable private UsbStatusListener listener;
    private UsbStatus reported = UsbStatus.CONNECTED;

    @Inject
    public PollingUsbWatcher(@NonNull UsbConnector connector, @NonNull Handler main) {
        this.connector = connector;
        this.main = main;
    }

    @Override
    public void start(@NonNull UsbStatusListener listener) {
        if (this.listener == null) {
            this.listener = listener;
            reported = UsbStatus.CONNECTED;
            main.post(this::look);
        }
    }

    @Override
    public void stop() {
        listener = null;
        main.removeCallbacksAndMessages(null);
    }

    private void look() {
        if (listener == null) {
            return;
        }
        UsbStatus status = connector.tryConnect();
        if (status != reported) {
            reported = status;
            listener.onUsbStatus(status);
        }
        main.postDelayed(this::look, LOOK_EVERY_MS);
    }
}
