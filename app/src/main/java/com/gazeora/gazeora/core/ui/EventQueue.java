package com.gazeora.gazeora.core.ui;

import androidx.annotation.NonNull;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * One-off events from a ViewModel. Queued rather than a single value, so events raised while
 * the screen is stopped all arrive; the screen marks them handled so they don't come again.
 */
public final class EventQueue<T> {
    private final MutableLiveData<List<T>> events = new MutableLiveData<>(Collections.emptyList());

    @NonNull
    public LiveData<List<T>> asLiveData() {
        return events;
    }

    public void post(@NonNull T event) {
        List<T> next = new ArrayList<>(current());
        next.add(event);
        events.setValue(Collections.unmodifiableList(next));
    }

    public void markHandled(@NonNull List<T> handled) {
        List<T> next = new ArrayList<>(current());
        next.removeAll(handled);
        events.setValue(Collections.unmodifiableList(next));
    }

    @NonNull
    private List<T> current() {
        List<T> value = events.getValue();
        return value == null ? Collections.emptyList() : value;
    }
}
