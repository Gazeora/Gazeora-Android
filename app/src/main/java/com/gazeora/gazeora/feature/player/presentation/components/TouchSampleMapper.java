package com.gazeora.gazeora.feature.player.presentation.components;

import android.view.MotionEvent;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.gazeora.gazeora.feature.player.domain.touch.PictureFit;
import com.gazeora.gazeora.feature.player.domain.touch.TouchPhase;
import com.gazeora.gazeora.feature.player.domain.touch.TouchSample;

/** Mapper pattern: an Android touch on the view as a touch on the picture fitted into it. */
final class TouchSampleMapper {
    private static final int FIRST_POINTER = 0;
    private static final int NO_POINTER = -1;

    /** Null for the kinds of touch event that aren't a finger's (hover and the like). */
    @Nullable
    TouchSample map(@NonNull MotionEvent event, @NonNull PictureFit fit) {
        TouchPhase phase = phaseOf(event.getActionMasked());
        if (phase == null) {
            return null;
        }
        // A finger lifting is still in the event; the center is of those that stay.
        int lifted = phase == TouchPhase.EXTRA_UP ? event.getActionIndex() : NO_POINTER;
        float sumX = 0;
        float sumY = 0;
        int count = 0;
        for (int i = 0; i < event.getPointerCount(); i++) {
            if (i != lifted) {
                sumX += event.getX(i);
                sumY += event.getY(i);
                count++;
            }
        }
        float centerX = count == 0 ? event.getX(FIRST_POINTER) : sumX / count;
        float centerY = count == 0 ? event.getY(FIRST_POINTER) : sumY / count;
        return new TouchSample(phase, fit.toPictureX(event.getX(FIRST_POINTER)),
                fit.toPictureY(event.getY(FIRST_POINTER)), fit.toPictureX(centerX),
                fit.toPictureY(centerY));
    }

    @Nullable
    private static TouchPhase phaseOf(int action) {
        switch (action) {
            case MotionEvent.ACTION_DOWN:
                return TouchPhase.FIRST_DOWN;
            case MotionEvent.ACTION_MOVE:
                return TouchPhase.MOVE;
            case MotionEvent.ACTION_POINTER_DOWN:
                return TouchPhase.EXTRA_DOWN;
            case MotionEvent.ACTION_POINTER_UP:
                return TouchPhase.EXTRA_UP;
            case MotionEvent.ACTION_UP:
                return TouchPhase.LAST_UP;
            case MotionEvent.ACTION_CANCEL:
                return TouchPhase.CANCEL;
            default:
                return null;
        }
    }
}
