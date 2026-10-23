package com.gazeora.gazeora.feature.connect.domain.usecase;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.core.stream.domain.model.StreamProtocol;
import com.gazeora.gazeora.feature.connect.domain.repository.ConnectionPreferencesRepository;

import javax.inject.Inject;

/** Remembers what was typed in, so the form offers it next time. */
public final class SaveManualConnectionUseCase {
    private final ConnectionPreferencesRepository prefs;

    @Inject
    public SaveManualConnectionUseCase(@NonNull ConnectionPreferencesRepository prefs) {
        this.prefs = prefs;
    }

    public void execute(@NonNull StreamProtocol protocol) {
        prefs.saveProtocol(protocol);
    }

    public void execute(@NonNull StreamProtocol protocol, @NonNull String address) {
        prefs.saveProtocol(protocol);
        prefs.saveAddress(address.trim());
    }
}
