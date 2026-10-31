// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.data.decoder;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.Objects;

/** The decoders the native player uses; null where the device has none. */
public final class DecoderChoice {
    @Nullable private final String avc;
    @Nullable private final String hevc;
    private final boolean avcLowLatency;
    private final boolean hevcLowLatency;

    public DecoderChoice(@Nullable String avc, boolean avcLowLatency, @Nullable String hevc,
            boolean hevcLowLatency) {
        this.avc = avc;
        this.avcLowLatency = avcLowLatency;
        this.hevc = hevc;
        this.hevcLowLatency = hevcLowLatency;
    }

    @Nullable
    public String getAvc() {
        return avc;
    }

    public boolean isAvcLowLatency() {
        return avcLowLatency;
    }

    @Nullable
    public String getHevc() {
        return hevc;
    }

    public boolean isHevcLowLatency() {
        return hevcLowLatency;
    }

    @Override
    public boolean equals(@Nullable Object other) {
        if (this == other) {
            return true;
        }
        if (!(other instanceof DecoderChoice)) {
            return false;
        }
        DecoderChoice that = (DecoderChoice) other;
        return avcLowLatency == that.avcLowLatency && hevcLowLatency == that.hevcLowLatency
                && Objects.equals(avc, that.avc) && Objects.equals(hevc, that.hevc);
    }

    @Override
    public int hashCode() {
        return Objects.hash(avc, hevc, avcLowLatency, hevcLowLatency);
    }

    @NonNull
    @Override
    public String toString() {
        return "DecoderChoice{avc=" + avc + (avcLowLatency ? " (low latency)" : "")
                + ", hevc=" + hevc + (hevcLowLatency ? " (low latency)" : "") + "}";
    }
}
