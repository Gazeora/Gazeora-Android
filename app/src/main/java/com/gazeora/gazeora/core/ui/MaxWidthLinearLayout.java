// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.core.ui;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.widget.LinearLayout;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

/**
 * A LinearLayout that honours {@code android:maxWidth}, which LinearLayout itself ignores, so
 * the home screen's column stays readable on tablets and in landscape.
 */
public final class MaxWidthLinearLayout extends LinearLayout {
    private static final int NO_MAX_WIDTH = 0;
    private static final int[] ATTRS = {android.R.attr.maxWidth};

    private final int maxWidth;  // px

    public MaxWidthLinearLayout(@NonNull Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
        // Not try-with-resources: TypedArray is only AutoCloseable from API 31.
        TypedArray a = context.obtainStyledAttributes(attrs, ATTRS);
        try {
            maxWidth = a.getDimensionPixelSize(0, NO_MAX_WIDTH);
        } finally {
            a.recycle();
        }
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int width = MeasureSpec.getSize(widthMeasureSpec);
        if (maxWidth > NO_MAX_WIDTH && width > maxWidth) {
            widthMeasureSpec = MeasureSpec.makeMeasureSpec(maxWidth, MeasureSpec.EXACTLY);
        }
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }
}
