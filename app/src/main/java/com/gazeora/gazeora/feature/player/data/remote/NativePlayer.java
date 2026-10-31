// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.data.remote;

import android.content.res.AssetManager;
import android.os.Handler;
import android.view.Surface;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.gazeora.gazeora.core.nativebridge.NativeLibrary;
import com.gazeora.gazeora.core.stream.domain.model.StreamTarget;
import com.gazeora.gazeora.feature.player.data.decoder.DecoderChoice;
import com.gazeora.gazeora.feature.player.data.mapper.PlayerStatusMapper;
import com.gazeora.gazeora.feature.player.domain.model.PlayerStatus;
import com.gazeora.gazeora.feature.player.domain.model.PointerCommand;
import com.gazeora.gazeora.feature.player.domain.model.ScreenGeometry;
import com.gazeora.gazeora.feature.player.domain.model.StreamPicture;
import com.gazeora.gazeora.feature.player.domain.player.PlaybackListener;
import com.gazeora.gazeora.feature.player.domain.player.PlaybackSession;

/**
 * Adapter pattern: the native player (src/main/cpp: player.h) behind {@link PlaybackSession}.
 * The native side does networking, the protocols and decoding; this brings what happens back
 * to the main thread.
 */
final class NativePlayer implements PlaybackSession {
    private static final long NO_HANDLE = 0;

    static {
        NativeLibrary.load();
    }

    private final Handler main;
    private final PlaybackListener listener;
    private final PlayerStatusMapper statuses;
    private long handle = NO_HANDLE;

    NativePlayer(@NonNull Handler main, @NonNull PlaybackListener listener,
            @NonNull PlayerStatusMapper statuses) {
        this.main = main;
        this.listener = listener;
        this.statuses = statuses;
    }

    /**
     * The native side tracks the viewer's hands in a VR viewer itself, with the models in
     * {@code assets}, keeping what the GPU makes of them in {@code cacheDir}. With
     * {@code handsDebug}, it shows the hands on a flat picture too.
     */
    void start(@NonNull Surface surface, @NonNull StreamTarget target,
            @NonNull DecoderChoice decoders, @NonNull AssetManager assets,
            @NonNull String cacheDir, boolean handsDebug) {
        handle = nativeStart(surface, target.getProtocol().getId(), target.getHost(),
                target.getPort(), target.getPath(), decoders.getAvc(), decoders.getHevc(),
                decoders.isAvcLowLatency(), decoders.isHevcLowLatency(), assets, cacheDir,
                handsDebug);
        if (handle == NO_HANDLE) {
            main.post(() -> listener.onStatus(PlayerStatus.FAILED, null));
        }
    }

    @Override
    public void setScreen(@NonNull ScreenGeometry screen) {
        if (handle != NO_HANDLE) {
            nativeSetScreen(handle, screen.getQuarterTurns(), screen.getDotsPerInch());
        }
    }

    @Override
    public void release() {
        if (handle != NO_HANDLE) {
            nativeRelease(handle);  // the camera and hand tracking stop with it
            handle = NO_HANDLE;
        }
        main.removeCallbacksAndMessages(null);
    }

    @Override
    public void send(@NonNull PointerCommand command) {
        if (handle != NO_HANDLE) {
            nativeSendPointer(handle, command.getAction().getWireId(),
                    command.getButton().getWireId(), command.getX(), command.getY(),
                    command.getWheelVertical(), command.getWheelHorizontal());
        }
    }

    // From the native player's threads.

    @Keep
    private void onNativeStatus(int status, @Nullable String detail) {
        whilePlaying(() -> listener.onStatus(statuses.map(status), detail));
    }

    @Keep
    private void onNativeVideoSize(int width, int height) {
        whilePlaying(() -> listener.onVideoSize(width, height));
    }

    @Keep
    private void onNativeFirstFrame() {
        whilePlaying(listener::onFirstFrame);
    }

    @Keep
    private void onNativePicture(boolean vr, int width, int height) {
        StreamPicture picture = new StreamPicture(vr, width, height);
        whilePlaying(() -> listener.onPicture(picture));
    }

    private void whilePlaying(@NonNull Runnable action) {
        main.post(() -> {
            if (handle != NO_HANDLE) {
                action.run();
            }
        });
    }

    private native long nativeStart(@NonNull Surface surface, @NonNull String protocol,
            @Nullable String host, int port, @Nullable String path, @Nullable String avcDecoder,
            @Nullable String hevcDecoder, boolean avcLowLatency, boolean hevcLowLatency,
            @NonNull AssetManager assets, @NonNull String cacheDir, boolean handsDebug);

    private static native void nativeRelease(long handle);

    private static native void nativeSendPointer(long handle, int action, int button, float x,
            float y, int wheelVertical, int wheelHorizontal);

    private static native void nativeSetScreen(long handle, int quarterTurns, float dotsPerInch);
}
