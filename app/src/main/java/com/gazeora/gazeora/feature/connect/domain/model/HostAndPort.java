package com.gazeora.gazeora.feature.connect.domain.model;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.Objects;

/** A PC's address as typed in: a host, maybe with a port. */
public final class HostAndPort {
    public static final int NO_PORT = -1;

    private final String host;
    private final int port;

    public HostAndPort(@NonNull String host, int port) {
        this.host = Objects.requireNonNull(host);
        this.port = port;
    }

    @NonNull
    public String getHost() {
        return host;
    }

    public int getPortOr(int defaultPort) {
        return port == NO_PORT ? defaultPort : port;
    }

    @Override
    public boolean equals(@Nullable Object other) {
        if (this == other) {
            return true;
        }
        if (!(other instanceof HostAndPort)) {
            return false;
        }
        HostAndPort that = (HostAndPort) other;
        return port == that.port && host.equals(that.host);
    }

    @Override
    public int hashCode() {
        return Objects.hash(host, port);
    }

    @NonNull
    @Override
    public String toString() {
        return host + ":" + port;
    }
}
