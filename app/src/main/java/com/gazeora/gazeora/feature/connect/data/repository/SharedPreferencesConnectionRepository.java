package com.gazeora.gazeora.feature.connect.data.repository;

import android.content.SharedPreferences;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.core.stream.domain.model.StreamProtocol;
import com.gazeora.gazeora.feature.connect.domain.repository.ConnectionPreferencesRepository;

import java.util.Optional;

import javax.inject.Inject;

/** Repository pattern: the last manual connection, in SharedPreferences. */
public final class SharedPreferencesConnectionRepository
        implements ConnectionPreferencesRepository {
    private static final String KEY_ADDRESS = "address";
    private static final String KEY_PROTOCOL = "protocol";

    private final SharedPreferences prefs;

    @Inject
    public SharedPreferencesConnectionRepository(@NonNull SharedPreferences prefs) {
        this.prefs = prefs;
    }

    @NonNull
    @Override
    public Optional<String> getAddress() {
        return Optional.ofNullable(prefs.getString(KEY_ADDRESS, null));
    }

    @Override
    public void saveAddress(@NonNull String address) {
        prefs.edit().putString(KEY_ADDRESS, address).apply();
    }

    @NonNull
    @Override
    public Optional<StreamProtocol> getProtocol() {
        return StreamProtocol.fromId(prefs.getString(KEY_PROTOCOL, null));
    }

    @Override
    public void saveProtocol(@NonNull StreamProtocol protocol) {
        prefs.edit().putString(KEY_PROTOCOL, protocol.getId()).apply();
    }
}
