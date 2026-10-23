package com.gazeora.gazeora.core.ui;

import androidx.annotation.NonNull;
import androidx.annotation.StringRes;

import com.gazeora.gazeora.R;
import com.gazeora.gazeora.core.stream.domain.model.StreamProtocol;

import java.util.Collections;
import java.util.EnumMap;
import java.util.Map;

/** Each protocol's name for people. */
public final class StreamProtocolLabels {
    private static final Map<StreamProtocol, Integer> LABELS = createLabels();

    private StreamProtocolLabels() {
    }

    @StringRes
    public static int of(@NonNull StreamProtocol protocol) {
        Integer label = LABELS.get(protocol);
        if (label == null) {
            throw new IllegalStateException("No label for " + protocol);
        }
        return label;
    }

    @NonNull
    private static Map<StreamProtocol, Integer> createLabels() {
        Map<StreamProtocol, Integer> labels = new EnumMap<>(StreamProtocol.class);
        labels.put(StreamProtocol.RTSP_TCP, R.string.protocol_rtsp_tcp);
        labels.put(StreamProtocol.RTSP_UDP, R.string.protocol_rtsp_udp);
        labels.put(StreamProtocol.UDP, R.string.protocol_udp);
        labels.put(StreamProtocol.HTTP, R.string.protocol_http);
        labels.put(StreamProtocol.USB, R.string.protocol_usb);
        return Collections.unmodifiableMap(labels);
    }
}
