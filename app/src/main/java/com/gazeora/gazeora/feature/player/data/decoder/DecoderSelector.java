package com.gazeora.gazeora.feature.player.data.decoder;

import android.media.MediaFormat;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import javax.inject.Inject;

/** Picks the native player's decoders: the NDK can't list them. */
public final class DecoderSelector {
    private final DecoderCatalog catalog;

    @Inject
    public DecoderSelector(@NonNull DecoderCatalog catalog) {
        this.catalog = catalog;
    }

    @NonNull
    public DecoderChoice select() {
        String avc = catalog.decoderFor(MediaFormat.MIMETYPE_VIDEO_AVC).orElse(null);
        String hevc = catalog.decoderFor(MediaFormat.MIMETYPE_VIDEO_HEVC).orElse(null);
        return new DecoderChoice(avc, lowLatency(avc, MediaFormat.MIMETYPE_VIDEO_AVC),
                hevc, lowLatency(hevc, MediaFormat.MIMETYPE_VIDEO_HEVC));
    }

    private boolean lowLatency(@Nullable String decoder, @NonNull String mime) {
        return decoder != null && catalog.supportsLowLatency(decoder, mime);
    }
}
