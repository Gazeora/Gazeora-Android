// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.report.domain.usecase;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.core.device.domain.SystemDescription;
import com.gazeora.gazeora.feature.report.domain.model.IssueLink;
import com.gazeora.gazeora.feature.report.domain.model.ProblemReport;

import javax.inject.Inject;

/** The link that opens a new Gazeora for Android bug report, describing this app and phone. */
public final class CreateIssueLinkUseCase {
    // As the bug report form asks for the Android app's version
    private static final String APP_VERSION_PREFIX = "Android app ";

    private final SystemDescription system;
    private final IssueLink link;

    @Inject
    public CreateIssueLinkUseCase(@NonNull SystemDescription system, @NonNull IssueLink link) {
        this.system = system;
        this.link = link;
    }

    @NonNull
    public String execute(@NonNull String appVersion) {
        ProblemReport report = new ProblemReport(APP_VERSION_PREFIX + appVersion, system.describe());
        return link.forReport(report);
    }
}
