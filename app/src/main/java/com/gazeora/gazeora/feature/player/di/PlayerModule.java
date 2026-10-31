// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.di;

import android.view.Surface;

import com.gazeora.gazeora.feature.player.data.decoder.DecoderCatalog;
import com.gazeora.gazeora.feature.player.data.decoder.MediaCodecDecoderCatalog;
import com.gazeora.gazeora.feature.player.data.network.WifiLowLatencyLock;
import com.gazeora.gazeora.feature.player.data.remote.NativePlayerFactory;
import com.gazeora.gazeora.feature.player.domain.player.NetworkLatencyLock;
import com.gazeora.gazeora.feature.player.domain.player.PlaybackSessionFactory;
import com.gazeora.gazeora.feature.player.domain.touch.RemotePointer;
import com.gazeora.gazeora.feature.player.presentation.PlaybackController;

import dagger.Binds;
import dagger.Module;
import dagger.hilt.InstallIn;
import dagger.hilt.android.components.ViewModelComponent;

/** The player's bindings, one set per player screen. */
@Module
@InstallIn(ViewModelComponent.class)
public interface PlayerModule {
    @Binds
    PlaybackSessionFactory<Surface> bindPlaybackSessionFactory(NativePlayerFactory impl);

    @Binds
    NetworkLatencyLock bindNetworkLatencyLock(WifiLowLatencyLock impl);

    @Binds
    DecoderCatalog bindDecoderCatalog(MediaCodecDecoderCatalog impl);

    @Binds
    RemotePointer bindRemotePointer(PlaybackController impl);
}
