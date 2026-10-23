package com.gazeora.gazeora.feature.connect.presentation;

import androidx.annotation.NonNull;

/** Ask for the local network permission. */
final class RequestNetworkPermissionEvent extends ConnectEvent {
    @Override
    public void dispatch(@NonNull ConnectEventHandler handler) {
        handler.requestNetworkPermission();
    }
}
