package com.gazeora.gazeora.feature.connect.presentation;

import androidx.annotation.NonNull;

/** Visitor pattern: a one-off thing the connect screen has to do. */
public abstract class ConnectEvent {
    ConnectEvent() {
    }

    public abstract void dispatch(@NonNull ConnectEventHandler handler);
}
