package com.gazeora.gazeora.feature.connect.presentation;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.core.stream.domain.model.StreamTarget;

/** Start playing {@code target}. */
final class OpenPlayerEvent extends ConnectEvent {
    private final StreamTarget target;

    OpenPlayerEvent(@NonNull StreamTarget target) {
        this.target = target;
    }

    @Override
    public void dispatch(@NonNull ConnectEventHandler handler) {
        handler.openPlayer(target);
    }
}
