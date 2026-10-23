package com.gazeora.gazeora.core.ui;

import android.content.res.Resources;

import androidx.annotation.NonNull;
import androidx.annotation.StringRes;

/** Text from strings.xml, with its format arguments. */
final class ResourceText extends UiText {
    @StringRes private final int resId;
    private final Object[] args;

    ResourceText(@StringRes int resId, @NonNull Object[] args) {
        this.resId = resId;
        this.args = args.clone();
    }

    @NonNull
    @Override
    public String resolve(@NonNull Resources resources) {
        Object[] resolved = new Object[args.length];
        for (int i = 0; i < args.length; ++i) {
            resolved[i] = args[i] instanceof UiText
                    ? ((UiText) args[i]).resolve(resources)
                    : args[i];
        }
        return resources.getString(resId, resolved);
    }
}
