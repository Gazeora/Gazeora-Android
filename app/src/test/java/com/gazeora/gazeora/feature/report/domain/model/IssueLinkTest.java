// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.report.domain.model;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;

import org.junit.Test;

public class IssueLinkTest {
    private static final String NEW_ISSUE = "https://github.com/owner/repo/issues/new";
    private static final ProblemReport REPORT = new ProblemReport("v1.2.0", "Pixel 9 & co");

    @Test
    public void opensTheFormWithTheVersionAndSystem() {
        String link = new IssueLink(NEW_ISSUE, "1-bug_report.yml").forReport(REPORT);
        assertEquals(NEW_ISSUE + "?template=1-bug_report.yml&version=v1.2.0"
                + "&system=Pixel%209%20%26%20co", link);
    }

    @Test
    public void encodesSpacesAsPercentTwenty() {
        String link = new IssueLink(NEW_ISSUE, "1-bug_report.yml").forReport(REPORT);
        assertFalse(link.contains("+"));
        assertFalse(link.contains(" "));
    }
}
