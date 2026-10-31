// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.report.domain.model;

import androidx.annotation.NonNull;

import java.util.Objects;

/** What a problem report already knows: the app's version and the phone it runs on. */
public final class ProblemReport {
    private final String appVersion;
    private final String system;

    public ProblemReport(@NonNull String appVersion, @NonNull String system) {
        this.appVersion = appVersion;
        this.system = system;
    }

    @NonNull
    public String getAppVersion() {
        return appVersion;
    }

    @NonNull
    public String getSystem() {
        return system;
    }

    @Override
    public boolean equals(Object other) {
        if (this == other) {
            return true;
        }
        if (!(other instanceof ProblemReport)) {
            return false;
        }
        ProblemReport that = (ProblemReport) other;
        return appVersion.equals(that.appVersion) && system.equals(that.system);
    }

    @Override
    public int hashCode() {
        return Objects.hash(appVersion, system);
    }

    @NonNull
    @Override
    public String toString() {
        return "ProblemReport{" + appVersion + ", " + system + "}";
    }
}
