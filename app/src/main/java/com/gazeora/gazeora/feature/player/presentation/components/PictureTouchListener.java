// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.player.presentation.components;

import android.annotation.SuppressLint;
import android.view.MotionEvent;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.core.util.Consumer;

import com.gazeora.gazeora.feature.player.domain.model.StreamPicture;
import com.gazeora.gazeora.feature.player.domain.touch.PictureFit;
import com.gazeora.gazeora.feature.player.domain.touch.TouchSample;

/** Passes on touches on the stream's view as touches on the picture shown in it. */
public final class PictureTouchListener implements View.OnTouchListener {
    private final Consumer<TouchSample> onTouch;
    private final TouchSampleMapper mapper = new TouchSampleMapper();
    private StreamPicture picture = StreamPicture.NONE;

    public PictureTouchListener(@NonNull Consumer<TouchSample> onTouch) {
        this.onTouch = onTouch;
    }

    public void setPicture(@NonNull StreamPicture picture) {
        this.picture = picture;
    }

    // The picture isn't a clickable control: touches go to the PC, not to performClick.
    @SuppressLint("ClickableViewAccessibility")
    @Override
    public boolean onTouch(@NonNull View view, @NonNull MotionEvent event) {
        PictureFit fit = new PictureFit(view.getWidth(), view.getHeight(),
                picture.getAspectRatio());
        TouchSample sample = mapper.map(event, fit);
        if (sample != null) {
            onTouch.accept(sample);
        }
        return true;
    }
}
