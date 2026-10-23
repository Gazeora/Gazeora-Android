package com.gazeora.gazeora.core.stream.domain.model;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.Optional;

/**
 * How the PC streams. The ids match the PC's stream.ini, its discovery replies and the native
 * player.
 */
public enum StreamProtocol {
    RTSP_TCP("rtsp-tcp", true),
    RTSP_UDP("rtsp-udp", true),
    UDP("udp", true),
    HTTP("http", true),
    USB("usb", false);

    private final String id;
    private final boolean needsAddress;

    StreamProtocol(@NonNull String id, boolean needsAddress) {
        this.id = id;
        this.needsAddress = needsAddress;
    }

    @NonNull
    public String getId() {
        return id;
    }

    /** Whether the player needs the PC's address: everything but USB. */
    public boolean needsAddress() {
        return needsAddress;
    }

    @NonNull
    public static Optional<StreamProtocol> fromId(@Nullable String id) {
        for (StreamProtocol protocol : values()) {
            if (protocol.id.equals(id)) {
                return Optional.of(protocol);
            }
        }
        return Optional.empty();
    }
}
