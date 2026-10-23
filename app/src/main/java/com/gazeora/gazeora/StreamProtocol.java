package com.gazeora.gazeora;

import androidx.annotation.Nullable;
import androidx.annotation.StringRes;

/**
 * How the PC streams: the protocol chosen in Gazeora's settings. The ids are the ones the PC
 * uses in stream.ini and in discovery replies (Gazeora: stream_protocol.h), and the native
 * player takes them too (player.h).
 */
enum StreamProtocol {
    RTSP_TCP("rtsp-tcp", R.string.protocol_rtsp_tcp),
    RTSP_UDP("rtsp-udp", R.string.protocol_rtsp_udp),
    UDP("udp", R.string.protocol_udp),
    HTTP("http", R.string.protocol_http),
    USB("usb", R.string.protocol_usb);

    final String id;
    @StringRes final int label;

    StreamProtocol(String id, @StringRes int label) {
        this.id = id;
        this.label = label;
    }

    @Nullable
    static StreamProtocol fromId(@Nullable String id) {
        for (StreamProtocol protocol : values()) {
            if (protocol.id.equals(id)) {
                return protocol;
            }
        }
        return null;
    }

    /** Whether the player needs the PC's address: everything but USB. */
    boolean needsAddress() {
        return this != USB;
    }
}
