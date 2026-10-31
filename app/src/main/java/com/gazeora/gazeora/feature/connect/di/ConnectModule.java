// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.connect.di;

import android.content.Context;
import android.content.SharedPreferences;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.feature.connect.data.repository.SharedPreferencesConnectionRepository;
import com.gazeora.gazeora.feature.connect.domain.repository.ConnectionPreferencesRepository;

import dagger.Binds;
import dagger.Module;
import dagger.Provides;
import dagger.hilt.InstallIn;
import dagger.hilt.android.qualifiers.ApplicationContext;
import dagger.hilt.components.SingletonComponent;

/** The connect screen's bindings. */
@Module
@InstallIn(SingletonComponent.class)
public abstract class ConnectModule {
    private static final String PREFS_NAME = "connection";

    @Binds
    abstract ConnectionPreferencesRepository bindConnectionPreferences(
            SharedPreferencesConnectionRepository impl);

    @Provides
    @NonNull
    static SharedPreferences provideConnectionPrefs(@NonNull @ApplicationContext Context context) {
        return context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
    }
}
