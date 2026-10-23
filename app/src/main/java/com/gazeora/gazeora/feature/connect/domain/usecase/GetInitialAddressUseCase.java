package com.gazeora.gazeora.feature.connect.domain.usecase;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.core.device.domain.DeviceInfo;
import com.gazeora.gazeora.core.stream.domain.model.StreamDefaults;
import com.gazeora.gazeora.feature.connect.domain.repository.ConnectionPreferencesRepository;

import javax.inject.Inject;

/** The address to offer: the last one used, or on the emulator the PC it runs on. */
public final class GetInitialAddressUseCase {
    private static final String EMULATOR_ADDRESS =
            StreamDefaults.EMULATOR_HOST + ":" + StreamDefaults.PORT;
    private static final String NO_ADDRESS = "";

    private final ConnectionPreferencesRepository prefs;
    private final DeviceInfo device;

    @Inject
    public GetInitialAddressUseCase(@NonNull ConnectionPreferencesRepository prefs,
            @NonNull DeviceInfo device) {
        this.prefs = prefs;
        this.device = device;
    }

    @NonNull
    public String execute() {
        return prefs.getAddress()
                .orElseGet(() -> device.isEmulator() ? EMULATOR_ADDRESS : NO_ADDRESS);
    }
}
