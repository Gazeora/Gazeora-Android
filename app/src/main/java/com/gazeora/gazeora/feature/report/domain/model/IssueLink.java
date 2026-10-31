// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.report.domain.model;

import androidx.annotation.NonNull;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;

import javax.inject.Inject;

/**
 * A link to GitHub's new issue page that opens the bug report form with the report's fields
 * filled in. Blank issues are turned off, so a plain body would be dropped on the way to the form.
 */
public final class IssueLink {
    private static final String GAZEORA_ANDROID_ISSUES =
            "https://github.com/Gazeora/Gazeora-Android/issues/new";
    // The organization's bug report issue form (Gazeora/.github), and its field ids
    private static final String BUG_REPORT_TEMPLATE = "1-bug_report.yml";
    private static final String TEMPLATE_PARAMETER = "template";
    private static final String VERSION_FIELD = "version";
    private static final String SYSTEM_FIELD = "system";
    private static final String QUERY_START = "?";
    private static final String PARAMETER_SEPARATOR = "&";
    private static final String NAME_VALUE_SEPARATOR = "=";
    private static final String ENCODING = "UTF-8";
    private static final String FORM_SPACE = "+";
    private static final String URL_SPACE = "%20";

    private final String newIssueUrl;
    private final String issueTemplate;

    @Inject
    public IssueLink() {
        this(GAZEORA_ANDROID_ISSUES, BUG_REPORT_TEMPLATE);
    }

    public IssueLink(@NonNull String newIssueUrl, @NonNull String issueTemplate) {
        this.newIssueUrl = newIssueUrl;
        this.issueTemplate = issueTemplate;
    }

    @NonNull
    public String forReport(@NonNull ProblemReport report) {
        return newIssueUrl + QUERY_START + parameter(TEMPLATE_PARAMETER, issueTemplate)
                + PARAMETER_SEPARATOR + parameter(VERSION_FIELD, report.getAppVersion())
                + PARAMETER_SEPARATOR + parameter(SYSTEM_FIELD, report.getSystem());
    }

    @NonNull
    private static String parameter(@NonNull String name, @NonNull String value) {
        return name + NAME_VALUE_SEPARATOR + encode(value);
    }

    // URLEncoder writes form encoding, where a space is "+"; a URL wants "%20".
    @NonNull
    private static String encode(@NonNull String text) {
        try {
            return URLEncoder.encode(text, ENCODING).replace(FORM_SPACE, URL_SPACE);
        } catch (UnsupportedEncodingException e) {
            throw new IllegalStateException("UTF-8 is always supported", e);
        }
    }
}
