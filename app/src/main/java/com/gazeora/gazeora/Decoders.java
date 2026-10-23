package com.gazeora.gazeora;

import android.media.MediaCodecInfo;
import android.media.MediaCodecList;

import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

/**
 * Which video decoder the native player uses: the NDK can't list them. The usual ones, but the
 * Android emulator's "goldfish" decoders last: they hand decoding to the PC's GPU and reject
 * the stream's H.265, while the emulator's software decoders play it. On a phone this changes
 * nothing.
 */
final class Decoders {
    private Decoders() {
    }

    static boolean isGoldfish(String name) {
        return name.contains(".goldfish.");
    }

    /** The decoder for {@code mime}, or null if this device has none. */
    @Nullable
    static String forMime(String mime) {
        List<MediaCodecInfo> candidates = new ArrayList<>();
        for (MediaCodecInfo info : new MediaCodecList(MediaCodecList.REGULAR_CODECS)
                .getCodecInfos()) {
            if (info.isEncoder()) {
                continue;
            }
            for (String type : info.getSupportedTypes()) {
                if (type.equalsIgnoreCase(mime)) {
                    candidates.add(info);
                    break;
                }
            }
        }
        candidates.sort(Comparator.comparing(info -> isGoldfish(info.getName())));
        return candidates.isEmpty() ? null : candidates.get(0).getName();
    }

    /** Whether {@code decoder} can decode with low latency. */
    @RequiresApi(30)
    static boolean supportsLowLatency(String decoder, String mime) {
        for (MediaCodecInfo info : new MediaCodecList(MediaCodecList.REGULAR_CODECS)
                .getCodecInfos()) {
            if (info.getName().equals(decoder)) {
                return info.getCapabilitiesForType(mime).isFeatureSupported(
                        MediaCodecInfo.CodecCapabilities.FEATURE_LowLatency);
            }
        }
        return false;
    }
}
