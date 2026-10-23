package com.gazeora.gazeora;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.widget.LinearLayout;

import androidx.annotation.Nullable;

/**
 * A LinearLayout that honours {@code android:maxWidth}, which LinearLayout itself ignores, so
 * the home screen's column stays readable on tablets and in landscape.
 */
public class MaxWidthLinearLayout extends LinearLayout {
    private final int maxWidth;  // px; 0 for none

    public MaxWidthLinearLayout(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
        TypedArray a = context.obtainStyledAttributes(attrs, new int[] {android.R.attr.maxWidth});
        maxWidth = a.getDimensionPixelSize(0, 0);
        a.recycle();
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int width = MeasureSpec.getSize(widthMeasureSpec);
        if (maxWidth > 0 && width > maxWidth) {
            widthMeasureSpec = MeasureSpec.makeMeasureSpec(maxWidth, MeasureSpec.EXACTLY);
        }
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }
}
