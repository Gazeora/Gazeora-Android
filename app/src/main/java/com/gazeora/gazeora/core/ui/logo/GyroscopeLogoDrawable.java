// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.core.ui.logo;

import android.animation.ValueAnimator;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.drawable.Animatable;
import android.graphics.drawable.Drawable;
import android.view.animation.LinearInterpolator;

import androidx.annotation.ColorRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.content.res.AppCompatResources;
import androidx.core.content.ContextCompat;

import com.gazeora.gazeora.R;

import java.util.Objects;

/**
 * The logo with its two rings spinning like a gyroscope: the globe about the vertical line, the
 * orbit about a line rising at 45 degrees. Each ring's far half passes behind the eye.
 */
public final class GyroscopeLogoDrawable extends Drawable implements Animatable {
    // The logo drawables' 108-unit viewport, holding the 256-unit art.
    private static final float VIEWPORT = 108f;
    private static final float ART_CENTER = 128f;
    private static final double GLOBE_RADIUS = 98.5;
    private static final double ORBIT_RADIUS = 98.4;
    private static final double ORBIT_TILT = Math.acos(25.5 / ORBIT_RADIUS);
    private static final double ORBIT_ROLL = Math.toRadians(-23.75);
    private static final double GLOBE_AXIS = Math.toRadians(90);
    private static final double ORBIT_AXIS = Math.toRadians(45);
    private static final float GLOBE_STROKE = 9f;
    private static final float ORBIT_STROKE = 7.5f;
    private static final float ORBIT_EDGE_STROKE = 13.5f;

    // Turns per cycle; whole numbers, so both rings are home together at each cycle's end.
    private static final int GLOBE_TURNS = 1;
    private static final int ORBIT_TURNS = -1;
    private static final double FULL_TURN = 2 * Math.PI;

    private final LogoPlacement placement;
    private final Drawable core;
    private final SpinningRing globe = new SpinningRing(
            ART_CENTER, ART_CENTER, GLOBE_RADIUS, 0, 0, GLOBE_AXIS);
    private final SpinningRing orbit = new SpinningRing(
            ART_CENTER, ART_CENTER, ORBIT_RADIUS, ORBIT_TILT, ORBIT_ROLL, ORBIT_AXIS);
    private final Paint globePaint;
    private final Paint orbitPaint;
    private final Paint orbitEdgePaint;
    private final Path globeBack = new Path();
    private final Path globeFront = new Path();
    private final Path orbitBack = new Path();
    private final Path orbitFront = new Path();
    private final ValueAnimator spin = ValueAnimator.ofFloat(0f, 1f);
    private boolean started;

    public GyroscopeLogoDrawable(@NonNull Context context, @NonNull LogoPlacement placement) {
        this.placement = placement;
        core = Objects.requireNonNull(
                AppCompatResources.getDrawable(context, placement.getCore()));
        globePaint = ringPaint(context, R.color.gazeora_accent, GLOBE_STROKE);
        orbitPaint = ringPaint(context, R.color.gazeora_accent_end, ORBIT_STROKE);
        // Dark edging under the orbit's near half, so it reads as passing in front.
        orbitEdgePaint = ringPaint(context, R.color.gazeora_window_bg, ORBIT_EDGE_STROKE);
        spin.setDuration(cycleMs(context));
        spin.setRepeatCount(ValueAnimator.INFINITE);
        spin.setInterpolator(new LinearInterpolator());
        spin.addUpdateListener(animation -> invalidateSelf());
    }

    /** How long both rings take to come home. */
    public static long cycleMs(@NonNull Context context) {
        return context.getResources().getInteger(R.integer.logo_gyroscope_cycle_ms);
    }

    @NonNull
    private static Paint ringPaint(@NonNull Context context, @ColorRes int color, float width) {
        Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeJoin(Paint.Join.ROUND);
        paint.setStrokeWidth(width);
        paint.setColor(ContextCompat.getColor(context, color));
        return paint;
    }

    @Override
    public void draw(@NonNull Canvas canvas) {
        double cycle = FULL_TURN * spin.getAnimatedFraction();
        globe.trace(cycle * GLOBE_TURNS, globeBack, globeFront);
        orbit.trace(cycle * ORBIT_TURNS, orbitBack, orbitFront);

        drawInArt(canvas, () -> {
            canvas.drawPath(globeBack, globePaint);
            canvas.drawPath(orbitBack, orbitPaint);
        });
        core.draw(canvas);
        drawInArt(canvas, () -> {
            canvas.drawPath(globeFront, globePaint);
            canvas.drawPath(orbitFront, orbitEdgePaint);
            canvas.drawPath(orbitFront, orbitPaint);
        });
    }

    /** Runs {@code drawing} in the art's 256-unit coordinates. */
    private void drawInArt(@NonNull Canvas canvas, @NonNull Runnable drawing) {
        Rect bounds = getBounds();
        int saved = canvas.save();
        canvas.translate(bounds.left, bounds.top);
        canvas.scale(bounds.width() / VIEWPORT, bounds.height() / VIEWPORT);
        canvas.translate(placement.getX(), placement.getY());
        canvas.scale(placement.getScale(), placement.getScale());
        drawing.run();
        canvas.restoreToCount(saved);
    }

    @Override
    protected void onBoundsChange(@NonNull Rect bounds) {
        core.setBounds(bounds);
    }

    @Override
    public int getIntrinsicWidth() {
        return core.getIntrinsicWidth();
    }

    @Override
    public int getIntrinsicHeight() {
        return core.getIntrinsicHeight();
    }

    @Override
    public void start() {
        started = true;
        spin.start();
    }

    @Override
    public void stop() {
        started = false;
        spin.cancel();
    }

    @Override
    public boolean isRunning() {
        return spin.isRunning();
    }

    /** Pauses while hidden (screen off, app in the background, view removed), saving battery. */
    @Override
    public boolean setVisible(boolean visible, boolean restart) {
        if (visible && started) {
            spin.resume();
        } else {
            spin.pause();
        }
        return super.setVisible(visible, restart);
    }

    @Override
    public void setAlpha(int alpha) {
        core.setAlpha(alpha);
        globePaint.setAlpha(alpha);
        orbitPaint.setAlpha(alpha);
        orbitEdgePaint.setAlpha(alpha);
        invalidateSelf();
    }

    @Override
    public void setColorFilter(@Nullable ColorFilter colorFilter) {
        core.setColorFilter(colorFilter);
        globePaint.setColorFilter(colorFilter);
        orbitPaint.setColorFilter(colorFilter);
        orbitEdgePaint.setColorFilter(colorFilter);
        invalidateSelf();
    }

    @Override
    public int getOpacity() {
        return PixelFormat.TRANSLUCENT;
    }
}
