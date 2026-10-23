package com.gazeora.gazeora;

import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.hardware.usb.UsbAccessory;
import android.hardware.usb.UsbManager;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelFileDescriptor;

import androidx.annotation.Nullable;

/**
 * Gets the native client the USB connection to the PC (Gazeora's USB protocol): waits for the
 * PC to switch this phone to accessory mode, asks for permission to use the accessory, opens
 * it and hands it over (NativePlayer.attachUsb). Looks again when the cable comes back.
 * Opening the accessory is Java-only; the native client does the rest.
 */
final class UsbAccessoryWatcher {
    /** What the PC calls itself as an accessory; res/xml/accessory_filter.xml matches it too. */
    static final String MANUFACTURER = "Gazeora";
    static final String MODEL = "Gazeora Stream";
    private static final String ACTION_PERMISSION = "com.gazeora.gazeora.USB_PERMISSION";
    private static final long LOOK_EVERY_MS = 500;

    interface Listener {
        /** On the main thread: what to show while there's no connection; null once there is. */
        void onUsbStatus(@Nullable String text);
    }

    private final Context context;
    private final UsbManager usb;
    private final Listener listener;
    private final Handler main = new Handler(Looper.getMainLooper());
    private boolean running;
    private boolean askedPermission;
    private String shown;

    UsbAccessoryWatcher(Context context, Listener listener) {
        this.context = context.getApplicationContext();
        usb = (UsbManager) this.context.getSystemService(Context.USB_SERVICE);
        this.listener = listener;
    }

    /** The Gazeora accessory, if the PC has switched this phone to it. */
    @Nullable
    static UsbAccessory findAccessory(UsbManager usb) {
        UsbAccessory[] accessories = usb.getAccessoryList();
        if (accessories != null) {
            for (UsbAccessory accessory : accessories) {
                if (MANUFACTURER.equals(accessory.getManufacturer())
                        && MODEL.equals(accessory.getModel())) {
                    return accessory;
                }
            }
        }
        return null;
    }

    void start() {
        if (!running) {
            running = true;
            shown = null;
            main.post(this::look);
        }
    }

    void stop() {
        running = false;
        main.removeCallbacksAndMessages(null);
    }

    private void look() {
        if (!running) {
            return;
        }
        String status = null;
        if (!NativePlayer.usbOpen()) {
            UsbAccessory accessory = findAccessory(usb);
            if (accessory == null) {
                status = context.getString(R.string.player_usb_waiting);
            } else if (!usb.hasPermission(accessory)) {
                status = context.getString(R.string.player_usb_permission);
                if (!askedPermission) {
                    askedPermission = true;
                    Intent intent = new Intent(ACTION_PERMISSION).setPackage(context.getPackageName());
                    usb.requestPermission(accessory, PendingIntent.getBroadcast(
                            context, 0, intent, PendingIntent.FLAG_MUTABLE));
                }
            } else {
                askedPermission = false;
                ParcelFileDescriptor descriptor = usb.openAccessory(accessory);
                if (descriptor == null) {
                    status = context.getString(R.string.player_usb_open_failed);
                } else {
                    NativePlayer.attachUsb(descriptor.detachFd());
                }
            }
        }
        if (status == null ? shown != null : !status.equals(shown)) {
            shown = status;
            listener.onUsbStatus(status);
        }
        main.postDelayed(this::look, LOOK_EVERY_MS);
    }
}
