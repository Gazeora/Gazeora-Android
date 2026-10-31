// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.data.decoder;

import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.os.Build;

import androidx.annotation.NonNull;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Optional;

import javax.inject.Inject;

/**
 * Adapter pattern: {@link MediaCodecList} behind {@link DecoderCatalog}. The emulator's
 * "goldfish" decoders go last: they hand decoding to the PC's GPU and reject the stream's
 * H.265, while the emulator's software decoders play it. On a phone this changes nothing.
 */
public final class MediaCodecDecoderCatalog implements DecoderCatalog {
    private static final String GOLDFISH = ".goldfish.";

    @Inject
    public MediaCodecDecoderCatalog() {
    }

    @NonNull
    @Override
    public Optional<String> decoderFor(@NonNull String mime) {
        List<MediaCodecInfo> candidates = new ArrayList<>();
        for (MediaCodecInfo info : codecs()) {
            if (!info.isEncoder() && supports(info, mime)) {
                candidates.add(info);
            }
        }
        candidates.sort(Comparator.comparing(info -> info.getName().contains(GOLDFISH)));
        return candidates.stream().findFirst().map(MediaCodecInfo::getName);
    }

    @Override
    public boolean supportsLowLatency(@NonNull String decoder, @NonNull String mime) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) {
            return false;
        }
        for (MediaCodecInfo info : codecs()) {
            if (info.getName().equals(decoder)) {
                return info.getCapabilitiesForType(mime).isFeatureSupported(
                        MediaCodecInfo.CodecCapabilities.FEATURE_LowLatency);
            }
        }
        return false;
    }

    @NonNull
    private static MediaCodecInfo[] codecs() {
        return new MediaCodecList(MediaCodecList.REGULAR_CODECS).getCodecInfos();
    }

    private static boolean supports(@NonNull MediaCodecInfo info, @NonNull String mime) {
        for (String type : info.getSupportedTypes()) {
            if (type.equalsIgnoreCase(mime)) {
                return true;
            }
        }
        return false;
    }
}
