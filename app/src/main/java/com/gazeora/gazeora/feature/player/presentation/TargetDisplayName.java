package com.gazeora.gazeora.feature.player.presentation;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.R;
import com.gazeora.gazeora.core.stream.domain.model.StreamTarget;
import com.gazeora.gazeora.core.ui.UiText;

/** The PC being played, for people: its name, else its address, else "USB". */
final class TargetDisplayName {
    private static final String HOST_PORT_SEPARATOR = ":";

    private TargetDisplayName() {
    }

    @NonNull
    static UiText of(@NonNull StreamTarget target) {
        String name = target.getName();
        if (name != null) {
            return UiText.of(name);
        }
        return target.getProtocol().needsAddress()
                ? UiText.of(target.getHost() + HOST_PORT_SEPARATOR + target.getPort())
                : UiText.of(R.string.protocol_usb);
    }
}
