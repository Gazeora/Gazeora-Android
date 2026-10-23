package com.gazeora.gazeora.core.ui;

import android.widget.Filter;

import androidx.annotation.NonNull;

/** A filter that keeps every item, whatever was typed. */
final class PassThroughFilter extends Filter {
    private final Object[] items;
    private final Runnable onPublish;

    PassThroughFilter(@NonNull Object[] items, @NonNull Runnable onPublish) {
        this.items = items.clone();
        this.onPublish = onPublish;
    }

    @Override
    protected FilterResults performFiltering(CharSequence constraint) {
        FilterResults results = new FilterResults();
        results.values = items;
        results.count = items.length;
        return results;
    }

    @Override
    protected void publishResults(CharSequence constraint, FilterResults results) {
        onPublish.run();
    }
}
