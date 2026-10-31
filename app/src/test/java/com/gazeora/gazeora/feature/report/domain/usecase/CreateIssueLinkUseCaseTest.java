// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.report.domain.usecase;

import static org.junit.Assert.assertEquals;

import com.gazeora.gazeora.feature.report.domain.model.IssueLink;
import com.gazeora.gazeora.feature.report.domain.model.ProblemReport;

import org.junit.Test;

public class CreateIssueLinkUseCaseTest {
    private static final String SYSTEM = "Google Pixel 9, Android 16 (API 36), arm64-v8a";

    @Test
    public void describesThisAppAndPhone() {
        IssueLink link = new IssueLink();
        String created = new CreateIssueLinkUseCase(() -> SYSTEM, link).execute("v1.0.1");
        assertEquals(link.forReport(new ProblemReport("Android app v1.0.1", SYSTEM)), created);
    }
}
