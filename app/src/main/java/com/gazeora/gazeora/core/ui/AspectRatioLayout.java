// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.core.ui;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.FrameLayout;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

/**
 * Fits its child into the space it's given at the picture's aspect ratio, centred, so the
 * stream shows whole and undistorted with black bars where the shapes differ.
 */
public final class AspectRatioLayout extends FrameLayout {
    private static final float UNKNOWN = 0f;

    private float aspectRatio = UNKNOWN;  // width over height; unknown fills the space

    public AspectRatioLayout(@NonNull Context context) {
        super(context);
    }

    public AspectRatioLayout(@NonNull Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
    }

    public void setAspectRatio(float aspectRatio) {
        if (this.aspectRatio != aspectRatio) {
            this.aspectRatio = aspectRatio;
            requestLayout();
        }
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int width = MeasureSpec.getSize(widthMeasureSpec);
        int height = MeasureSpec.getSize(heightMeasureSpec);
        if (aspectRatio > UNKNOWN && width > 0 && height > 0) {
            if (width > height * aspectRatio) {
                width = Math.round(height * aspectRatio);
            } else {
                height = Math.round(width / aspectRatio);
            }
            widthMeasureSpec = MeasureSpec.makeMeasureSpec(width, MeasureSpec.EXACTLY);
            heightMeasureSpec = MeasureSpec.makeMeasureSpec(height, MeasureSpec.EXACTLY);
        }
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }
}
