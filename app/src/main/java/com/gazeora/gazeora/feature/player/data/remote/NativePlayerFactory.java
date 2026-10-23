package com.gazeora.gazeora.feature.player.data.remote;

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

/** Factory pattern: a native player per stream, with this device's decoders. */
public final class NativePlayerFactory implements PlaybackSessionFactory<Surface> {
    private final Provider<Handler> handlers;
    private final DecoderSelector decoders;
    private final PlayerStatusMapper statuses;

    @Inject
    public NativePlayerFactory(@NonNull Provider<Handler> handlers,
            @NonNull DecoderSelector decoders, @NonNull PlayerStatusMapper statuses) {
        this.handlers = handlers;
        this.decoders = decoders;
        this.statuses = statuses;
    }

    @NonNull
    @Override
    public PlaybackSession open(@NonNull Surface surface, @NonNull StreamTarget target,
            @NonNull PlaybackListener listener) {
        NativePlayer player = new NativePlayer(handlers.get(), listener, statuses);
        player.start(surface, target, decoders.select());
        return player;
    }
}
