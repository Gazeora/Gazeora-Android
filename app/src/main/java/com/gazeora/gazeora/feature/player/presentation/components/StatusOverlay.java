// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.presentation.components;

import android.app.Activity;
import android.content.res.Resources;
import android.view.View;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.gazeora.gazeora.R;
import com.gazeora.gazeora.core.ui.UiText;
import com.gazeora.gazeora.core.ui.Views;
import com.gazeora.gazeora.feature.player.presentation.StatusView;

/** The message over the stream while there's no picture. */
public final class StatusOverlay implements StatusView {
    private final Resources resources;
    private final View status;
    private final View progress;
    private final TextView statusText;
    private final TextView statusDetail;
    private final View retry;

    public StatusOverlay(@NonNull Activity activity, @NonNull Runnable onRetry) {
        resources = activity.getResources();
        status = activity.findViewById(R.id.status);
        progress = activity.findViewById(R.id.progress);
        statusText = activity.findViewById(R.id.status_text);
        statusDetail = activity.findViewById(R.id.status_detail);
        retry = activity.findViewById(R.id.retry);
        retry.setOnClickListener(v -> onRetry.run());
    }

    @Override
    public void hide() {
        status.setVisibility(View.GONE);
    }

    @Override
    public void showProgress(@NonNull UiText message) {
        show(message, null, false);
    }

    @Override
    public void showFailure(@NonNull UiText message, @Nullable UiText detail) {
        show(message, detail, true);
    }

    private void show(@NonNull UiText message, @Nullable UiText detail, boolean failed) {
        status.setVisibility(View.VISIBLE);
        progress.setVisibility(Views.visibleIf(!failed));
        retry.setVisibility(Views.visibleIf(failed));
        statusText.setText(message.resolve(resources));
        statusDetail.setText(detail == null ? null : detail.resolve(resources));
        statusDetail.setVisibility(Views.visibleIf(detail != null));
    }
}
