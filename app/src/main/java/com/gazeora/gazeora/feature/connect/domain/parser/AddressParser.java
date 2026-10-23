package com.gazeora.gazeora.feature.connect.domain.parser;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.feature.connect.domain.model.HostAndPort;

import java.util.Optional;
import java.util.regex.Pattern;

import javax.inject.Inject;

/**
 * Reads "host[:port]" the way a URI's authority reads, with or without a scheme in front.
 * IPv6 addresses aren't supported: their host keeps its colons.
 */
public final class AddressParser {
    private static final Pattern SCHEME = Pattern.compile("^[a-zA-Z]+://");
    private static final String AUTHORITY_ENDS = "/?#";
    private static final char USER_INFO_END = '@';
    private static final char PORT_SEPARATOR = ':';
    private static final char IPV6_END = ']';
    private static final int NOT_FOUND = -1;

    @Inject
    public AddressParser() {
    }

    @NonNull
    public Optional<HostAndPort> parse(@NonNull String input) {
        String authority = authorityOf(SCHEME.matcher(input).replaceFirst(""));
        String hostAndPort = authority.substring(authority.lastIndexOf(USER_INFO_END) + 1);
        int separator = portSeparatorOf(hostAndPort);
        String host = separator == NOT_FOUND ? hostAndPort : hostAndPort.substring(0, separator);
        if (host.isEmpty() || host.indexOf(PORT_SEPARATOR) != NOT_FOUND) {
            return Optional.empty();
        }
        int port = separator == NOT_FOUND
                ? HostAndPort.NO_PORT
                : parsePort(hostAndPort.substring(separator + 1));
        return Optional.of(new HostAndPort(host, port));
    }

    @NonNull
    private static String authorityOf(@NonNull String address) {
        for (int i = 0; i < address.length(); ++i) {
            if (AUTHORITY_ENDS.indexOf(address.charAt(i)) != NOT_FOUND) {
                return address.substring(0, i);
            }
        }
        return address;
    }

    private static int portSeparatorOf(@NonNull String hostAndPort) {
        int separator = hostAndPort.lastIndexOf(PORT_SEPARATOR);
        return separator > hostAndPort.lastIndexOf(IPV6_END) ? separator : NOT_FOUND;
    }

    private static int parsePort(@NonNull String port) {
        try {
            return Integer.parseInt(port);
        } catch (NumberFormatException notANumber) {
            return HostAndPort.NO_PORT;
        }
    }
}
