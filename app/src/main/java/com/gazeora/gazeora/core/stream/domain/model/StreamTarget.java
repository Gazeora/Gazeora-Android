// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.core.stream.domain.model;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.Objects;

/** Where to play from: the protocol, and for the network ones the PC's address. */
public final class StreamTarget {
    private static final int NO_PORT = 0;

    private final StreamProtocol protocol;
    @Nullable private final String host;
    private final int port;
    @Nullable private final String path;
    @Nullable private final String name;

    public StreamTarget(@NonNull StreamProtocol protocol, @Nullable String host, int port,
            @Nullable String path, @Nullable String name) {
        this.protocol = Objects.requireNonNull(protocol);
        this.host = host;
        this.port = port;
        this.path = path;
        this.name = name;
    }

    /** The PC on the USB cable. */
    @NonNull
    public static StreamTarget usb() {
        return new StreamTarget(StreamProtocol.USB, null, NO_PORT, null, null);
    }

    /** The same PC, now streaming over USB; the address stays for a later move back. */
    @NonNull
    public StreamTarget switchedToUsb(@NonNull String pcName) {
        return new StreamTarget(StreamProtocol.USB, host, port, path, pcName);
    }

    @NonNull
    public StreamProtocol getProtocol() {
        return protocol;
    }

    @Nullable
    public String getHost() {
        return host;
    }

    public int getPort() {
        return port;
    }

    /** Null for the protocol's usual one. */
    @Nullable
    public String getPath() {
        return path;
    }

    /** The PC's name; null when it was entered by address. */
    @Nullable
    public String getName() {
        return name;
    }

    @Override
    public boolean equals(@Nullable Object other) {
        if (this == other) {
            return true;
        }
        if (!(other instanceof StreamTarget)) {
            return false;
        }
        StreamTarget that = (StreamTarget) other;
        return port == that.port && protocol == that.protocol && Objects.equals(host, that.host)
                && Objects.equals(path, that.path) && Objects.equals(name, that.name);
    }

    @Override
    public int hashCode() {
        return Objects.hash(protocol, host, port, path, name);
    }

    @NonNull
    @Override
    public String toString() {
        return "StreamTarget{" + protocol + ", " + host + ":" + port + ", path=" + path
                + ", name=" + name + "}";
    }
}
