// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.data.remote;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.os.Handler;
import android.view.Surface;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.core.stream.domain.model.StreamTarget;
import com.gazeora.gazeora.feature.player.data.decoder.DecoderSelector;
import com.gazeora.gazeora.feature.player.data.mapper.PlayerStatusMapper;
import com.gazeora.gazeora.feature.player.domain.player.PlaybackListener;
import com.gazeora.gazeora.feature.player.domain.player.PlaybackSession;
import com.gazeora.gazeora.feature.player.domain.player.PlaybackSessionFactory;

import javax.inject.Inject;
import javax.inject.Provider;

import dagger.hilt.android.qualifiers.ApplicationContext;

/** Factory pattern: a native player per stream, with this device's decoders. */
public final class NativePlayerFactory implements PlaybackSessionFactory<Surface> {
    private final Provider<Handler> handlers;
    private final DecoderSelector decoders;
    private final PlayerStatusMapper statuses;
    private final Context context;

    @Inject
    public NativePlayerFactory(@NonNull Provider<Handler> handlers,
            @NonNull DecoderSelector decoders, @NonNull PlayerStatusMapper statuses,
            @NonNull @ApplicationContext Context context) {
        this.handlers = handlers;
        this.decoders = decoders;
        this.statuses = statuses;
        this.context = context;
    }

    @NonNull
    @Override
    public PlaybackSession open(@NonNull Surface surface, @NonNull StreamTarget target,
            @NonNull PlaybackListener listener) {
        NativePlayer player = new NativePlayer(handlers.get(), listener, statuses);
        // Debuggable builds show the hands on a flat picture too, for debugging them.
        boolean debuggable = (context.getApplicationInfo().flags & ApplicationInfo.FLAG_DEBUGGABLE) != 0;
        player.start(surface, target, decoders.select(), context.getAssets(),
                context.getCodeCacheDir().getAbsolutePath(), debuggable);
        return player;
    }
}
