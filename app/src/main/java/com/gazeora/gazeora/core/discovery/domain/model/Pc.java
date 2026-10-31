// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.core.discovery.domain.model;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.gazeora.gazeora.core.stream.domain.model.StreamProtocol;
import com.gazeora.gazeora.core.stream.domain.model.StreamTarget;

import java.util.Objects;

/** A PC that answered discovery: where it streams and how. */
public final class Pc {
    private final String name;
    private final String host;
    private final StreamProtocol protocol;
    private final int port;
    private final String path;
    private final String codec;

    private Pc(@NonNull Builder builder) {
        name = Objects.requireNonNull(builder.name);
        host = Objects.requireNonNull(builder.host);
        protocol = Objects.requireNonNull(builder.protocol);
        port = builder.port;
        path = Objects.requireNonNull(builder.path);
        codec = Objects.requireNonNull(builder.codec);
    }

    @NonNull
    public String getName() {
        return name;
    }

    @NonNull
    public String getHost() {
        return host;
    }

    @NonNull
    public StreamProtocol getProtocol() {
        return protocol;
    }

    public int getPort() {
        return port;
    }

    @NonNull
    public String getPath() {
        return path;
    }

    @NonNull
    public String getCodec() {
        return codec;
    }

    /** Identifies the PC across protocol changes. */
    @NonNull
    public String getKey() {
        return host + " " + name;
    }

    /** What to play to watch this PC. */
    @NonNull
    public StreamTarget toStreamTarget() {
        return protocol.needsAddress()
                ? new StreamTarget(protocol, host, port, path, name)
                : StreamTarget.usb();
    }

    @Override
    public boolean equals(@Nullable Object other) {
        if (this == other) {
            return true;
        }
        if (!(other instanceof Pc)) {
            return false;
        }
        Pc that = (Pc) other;
        return port == that.port && name.equals(that.name) && host.equals(that.host)
                && protocol == that.protocol && path.equals(that.path)
                && codec.equals(that.codec);
    }

    @Override
    public int hashCode() {
        return Objects.hash(name, host, protocol, port, path, codec);
    }

    @NonNull
    @Override
    public String toString() {
        return "Pc{" + name + " at " + host + ":" + port + ", " + protocol + ", path=" + path
                + ", codec=" + codec + "}";
    }

    // Builder pattern: six fields, most of them strings that are easy to mix up.
    public static final class Builder {
        private String name;
        private String host;
        private StreamProtocol protocol;
        private int port;
        private String path = "";
        private String codec = "";

        @NonNull
        public Builder name(@NonNull String name) {
            this.name = name;
            return this;
        }

        @NonNull
        public Builder host(@NonNull String host) {
            this.host = host;
            return this;
        }

        @NonNull
        public Builder protocol(@NonNull StreamProtocol protocol) {
            this.protocol = protocol;
            return this;
        }

        @NonNull
        public Builder port(int port) {
            this.port = port;
            return this;
        }

        @NonNull
        public Builder path(@NonNull String path) {
            this.path = path;
            return this;
        }

        @NonNull
        public Builder codec(@NonNull String codec) {
            this.codec = codec;
            return this;
        }

        @NonNull
        public Pc build() {
            return new Pc(this);
        }
    }
}
