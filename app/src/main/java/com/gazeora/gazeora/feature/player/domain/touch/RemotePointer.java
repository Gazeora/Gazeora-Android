package com.gazeora.gazeora.feature.player.domain.touch;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.feature.player.domain.model.PointerCommand;

/** The PC's mouse, driven from the phone. Commands while nothing plays are dropped. */
public interface RemotePointer {
    void send(@NonNull PointerCommand command);
}
