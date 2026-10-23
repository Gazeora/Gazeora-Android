package com.gazeora.gazeora.feature.player.presentation;

import android.view.Surface;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.gazeora.gazeora.core.stream.domain.model.StreamTarget;
import com.gazeora.gazeora.core.usb.domain.UsbStatusListener;
import com.gazeora.gazeora.core.usb.domain.UsbWatcher;
import com.gazeora.gazeora.feature.player.domain.model.PointerCommand;
import com.gazeora.gazeora.feature.player.domain.player.NetworkLatencyLock;
import com.gazeora.gazeora.feature.player.domain.player.PlaybackListener;
import com.gazeora.gazeora.feature.player.domain.player.PlaybackSession;
import com.gazeora.gazeora.feature.player.domain.player.PlaybackSessionFactory;
import com.gazeora.gazeora.feature.player.domain.touch.RemotePointer;

import javax.inject.Inject;

import dagger.hilt.android.scopes.ViewModelScoped;

/**
 * Runs one stream at a time onto the surface, with what keeps its connection up; touches go
 * to the one playing.
 */
@ViewModelScoped
public final class PlaybackController implements RemotePointer {
    private final PlaybackSessionFactory<Surface> sessions;
    private final UsbWatcher usbWatcher;
    private final NetworkLatencyLock networkLock;
    @Nullable private Surface surface;
    @Nullable private PlaybackSession session;

    @Inject
    public PlaybackController(@NonNull PlaybackSessionFactory<Surface> sessions,
            @NonNull UsbWatcher usbWatcher, @NonNull NetworkLatencyLock networkLock) {
        this.sessions = sessions;
        this.usbWatcher = usbWatcher;
        this.networkLock = networkLock;
    }

    public void setSurface(@Nullable Surface surface) {
        this.surface = surface;
    }

    /** Whether a new stream started: not without a surface, nor while one plays. */
    public boolean start(@NonNull StreamTarget target, @NonNull PlaybackListener listener,
            @NonNull UsbStatusListener usbListener) {
        if (session != null || surface == null) {
            return false;
        }
        session = sessions.open(surface, target, listener);
        if (target.getProtocol().needsAddress()) {
            networkLock.acquire();
        } else {
            usbWatcher.start(usbListener);
        }
        return true;
    }

    @Override
    public void send(@NonNull PointerCommand command) {
        if (session != null) {
            session.send(command);
        }
    }

    public void stop() {
        if (session != null) {
            session.release();
            session = null;
        }
        usbWatcher.stop();
        networkLock.release();
    }
}
