package com.gazeora.gazeora.core.ui;

import android.content.Context;
import android.widget.ArrayAdapter;
import android.widget.Filter;

import androidx.annotation.LayoutRes;
import androidx.annotation.NonNull;

/**
 * Offers all its items whatever the field shows. An ArrayAdapter filters by the field's text,
 * so with a choice made (or the text restored after a rotation) it would offer only that one.
 */
public final class UnfilteredArrayAdapter extends ArrayAdapter<String> {
    private final Filter filter;

    public UnfilteredArrayAdapter(@NonNull Context context, @LayoutRes int layout,
            @NonNull String[] items) {
        super(context, layout, items);
        filter = new PassThroughFilter(items, this::notifyDataSetChanged);
    }

    @NonNull
    @Override
    public Filter getFilter() {
        return filter;
    }
}
