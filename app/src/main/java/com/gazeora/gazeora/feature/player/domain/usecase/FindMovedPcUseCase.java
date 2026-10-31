// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.domain.usecase;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.core.discovery.domain.model.Pc;
import com.gazeora.gazeora.core.stream.domain.model.StreamProtocol;
import com.gazeora.gazeora.core.stream.domain.model.StreamTarget;
import com.gazeora.gazeora.feature.player.domain.model.KnownUsbPcs;

import java.util.List;
import java.util.Optional;

import javax.inject.Inject;

/**
 * Follows the PC to another protocol: changing it in Gazeora restarts the stream, and the PC
 * then says so in discovery. The PC is the one playing (by name, or by address when typed in);
 * for USB, one that said it streams over USB.
 */
public final class FindMovedPcUseCase {
    @Inject
    public FindMovedPcUseCase() {
    }

    /** What to play now, if the PC moved. */
    @NonNull
    public Optional<StreamTarget> execute(@NonNull StreamTarget current, @NonNull List<Pc> pcs,
            @NonNull KnownUsbPcs knownUsbPcs) {
        Pc moved = null;
        for (Pc pc : pcs) {
            if (hasMoved(current, pc, knownUsbPcs)) {
                moved = pc;
            }
        }
        return moved == null ? Optional.empty() : Optional.of(follow(current, moved, knownUsbPcs));
    }

    private static boolean hasMoved(@NonNull StreamTarget current, @NonNull Pc pc,
            @NonNull KnownUsbPcs knownUsbPcs) {
        if (current.getProtocol() != StreamProtocol.USB) {
            boolean changed = pc.getProtocol() != current.getProtocol()
                    || pc.getPort() != current.getPort();
            return isSamePc(current, pc) && changed;
        }
        if (pc.getProtocol() == StreamProtocol.USB) {
            knownUsbPcs.remember(pc);
            return false;
        }
        return knownUsbPcs.contains(pc);
    }

    private static boolean isSamePc(@NonNull StreamTarget current, @NonNull Pc pc) {
        String name = current.getName();
        return pc.getHost().equals(current.getHost())
                && (name == null || pc.getName().equals(name));
    }

    @NonNull
    private static StreamTarget follow(@NonNull StreamTarget current, @NonNull Pc moved,
            @NonNull KnownUsbPcs knownUsbPcs) {
        if (moved.getProtocol().needsAddress()) {
            return moved.toStreamTarget();
        }
        // The PC switches this phone over when it's on the cable.
        knownUsbPcs.remember(moved);
        return current.switchedToUsb(moved.getName());
    }
}
