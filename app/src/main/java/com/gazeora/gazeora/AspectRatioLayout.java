package com.gazeora.gazeora;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.FrameLayout;

import androidx.annotation.Nullable;

/**
 * Fits its child into the space it's given at the picture's aspect ratio, centred, so the
 * stream shows whole and undistorted with black bars where the shapes differ.
 */
public class AspectRatioLayout extends FrameLayout {
    private float aspectRatio;  // width over height; 0 until known: fill the space

    public AspectRatioLayout(Context context) {
        super(context);
    }

    public AspectRatioLayout(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
    }

    void setAspectRatio(float aspectRatio) {
        if (this.aspectRatio != aspectRatio) {
            this.aspectRatio = aspectRatio;
            requestLayout();
        }
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int width = MeasureSpec.getSize(widthMeasureSpec);
        int height = MeasureSpec.getSize(heightMeasureSpec);
        if (aspectRatio > 0 && width > 0 && height > 0) {
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
