package com.gazeora.gazeora.core.ui;

import android.view.View;

/** Small helpers for views. */
public final class Views {
    private Views() {
    }

    public static int visibleIf(boolean visible) {
        return visible ? View.VISIBLE : View.GONE;
    }
}
