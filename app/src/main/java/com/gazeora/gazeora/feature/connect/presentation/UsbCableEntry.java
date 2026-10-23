package com.gazeora.gazeora.feature.connect.presentation;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.R;
import com.gazeora.gazeora.core.stream.domain.model.StreamTarget;
import com.gazeora.gazeora.core.ui.UiText;

/** The PC on the USB cable, whether or not discovery found it. */
final class UsbCableEntry extends PcEntry {
    @NonNull
    @Override
    public UiText getTitle() {
        return UiText.of(R.string.connect_usb_title);
    }

    @NonNull
    @Override
    public UiText getDetail() {
        return UiText.of(R.string.connect_usb_detail);
    }

    @NonNull
    @Override
    public StreamTarget getTarget() {
        return StreamTarget.usb();
    }
}
