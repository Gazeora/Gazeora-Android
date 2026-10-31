// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.connect.domain.usecase;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.core.stream.domain.model.StreamProtocol;
import com.gazeora.gazeora.feature.connect.domain.repository.ConnectionPreferencesRepository;

import org.junit.Test;

import java.util.Optional;

public class SaveManualConnectionUseCaseTest {
    private final FakePrefs prefs = new FakePrefs();
    private final SaveManualConnectionUseCase save = new SaveManualConnectionUseCase(prefs);

    @Test
    public void savesTheProtocolAlone() {
        save.execute(StreamProtocol.USB);

        assertEquals(Optional.of(StreamProtocol.USB), prefs.getProtocol());
        assertFalse(prefs.getAddress().isPresent());
    }

    @Test
    public void savesTheAddressTrimmed() {
        save.execute(StreamProtocol.HTTP, "  192.168.1.20:8554 ");

        assertEquals(Optional.of(StreamProtocol.HTTP), prefs.getProtocol());
        assertEquals(Optional.of("192.168.1.20:8554"), prefs.getAddress());
    }

    private static final class FakePrefs implements ConnectionPreferencesRepository {
        private String address;
        private StreamProtocol protocol;

        @NonNull
        @Override
        public Optional<String> getAddress() {
            return Optional.ofNullable(address);
        }

        @Override
        public void saveAddress(@NonNull String address) {
            this.address = address;
        }

        @NonNull
        @Override
        public Optional<StreamProtocol> getProtocol() {
            return Optional.ofNullable(protocol);
        }

        @Override
        public void saveProtocol(@NonNull StreamProtocol protocol) {
            this.protocol = protocol;
        }
    }
}
