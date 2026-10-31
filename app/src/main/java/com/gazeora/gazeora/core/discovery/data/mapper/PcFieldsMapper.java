// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.core.discovery.data.mapper;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.core.discovery.domain.model.Pc;
import com.gazeora.gazeora.core.stream.domain.model.StreamProtocol;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Optional;

import javax.inject.Inject;

/** Mapper pattern: the native search's flat fields to {@link Pc}s. */
public final class PcFieldsMapper {
    // Six strings a PC, in this order (jni.cpp).
    private static final int NAME = 0;
    private static final int HOST = 1;
    private static final int PROTOCOL = 2;
    private static final int PORT = 3;
    private static final int PATH = 4;
    private static final int CODEC = 5;
    private static final int FIELDS_PER_PC = 6;

    @Inject
    public PcFieldsMapper() {
    }

    /** Skips PCs with a protocol this app doesn't know, or a bad port. */
    @NonNull
    public List<Pc> map(@NonNull String[] fields) {
        List<Pc> pcs = new ArrayList<>();
        for (int at = 0; at + FIELDS_PER_PC <= fields.length; at += FIELDS_PER_PC) {
            mapOne(fields, at).ifPresent(pcs::add);
        }
        return Collections.unmodifiableList(pcs);
    }

    @NonNull
    private static Optional<Pc> mapOne(@NonNull String[] fields, int at) {
        Optional<StreamProtocol> protocol = StreamProtocol.fromId(fields[at + PROTOCOL]);
        if (!protocol.isPresent()) {
            return Optional.empty();
        }
        try {
            return Optional.of(new Pc.Builder()
                    .name(fields[at + NAME])
                    .host(fields[at + HOST])
                    .protocol(protocol.get())
                    .port(Integer.parseInt(fields[at + PORT]))
                    .path(fields[at + PATH])
                    .codec(fields[at + CODEC])
                    .build());
        } catch (NumberFormatException badPort) {
            return Optional.empty();
        }
    }
}
