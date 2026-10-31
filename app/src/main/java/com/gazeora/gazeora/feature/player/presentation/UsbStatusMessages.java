// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.presentation;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.R;
import com.gazeora.gazeora.core.usb.domain.UsbStatus;

import java.util.Collections;
import java.util.EnumMap;
import java.util.Map;
import java.util.Optional;

/** What to show while there's no USB connection yet. */
final class UsbStatusMessages {
    private static final Map<UsbStatus, Integer> MESSAGES = createMessages();

    private UsbStatusMessages() {
    }

    /** Empty once connected: nothing to wait for. */
    @NonNull
    static Optional<Integer> of(@NonNull UsbStatus status) {
        return Optional.ofNullable(MESSAGES.get(status));
    }

    @NonNull
    private static Map<UsbStatus, Integer> createMessages() {
        Map<UsbStatus, Integer> messages = new EnumMap<>(UsbStatus.class);
        messages.put(UsbStatus.WAITING_FOR_PC, R.string.player_usb_waiting);
        messages.put(UsbStatus.NEEDS_PERMISSION, R.string.player_usb_permission);
        messages.put(UsbStatus.OPEN_FAILED, R.string.player_usb_open_failed);
        return Collections.unmodifiableMap(messages);
    }
}
