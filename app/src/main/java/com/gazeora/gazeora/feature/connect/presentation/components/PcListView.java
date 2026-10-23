package com.gazeora.gazeora.feature.connect.presentation.components;

import android.app.Activity;
import android.content.res.Resources;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.R;
import com.gazeora.gazeora.core.ui.Views;
import com.gazeora.gazeora.feature.connect.presentation.PcEntry;
import com.gazeora.gazeora.feature.connect.presentation.PcListUiState;

/** Shows the PCs as cards, and how the search is going. */
public final class PcListView {
    /** What the user does with the list. */
    public interface Listener {
        void onEntrySelected(@NonNull PcEntry entry);

        void onAllowNetwork();
    }

    private final LayoutInflater inflater;
    private final Resources resources;
    private final Listener listener;
    private final LinearLayout list;
    private final View searching;
    private final TextView searchHint;
    private final View allowNetwork;

    public PcListView(@NonNull Activity activity, @NonNull Listener listener) {
        inflater = activity.getLayoutInflater();
        resources = activity.getResources();
        this.listener = listener;
        list = activity.findViewById(R.id.pc_list);
        searching = activity.findViewById(R.id.searching);
        searchHint = activity.findViewById(R.id.search_hint);
        allowNetwork = activity.findViewById(R.id.allow_network);
        allowNetwork.setOnClickListener(v -> listener.onAllowNetwork());
    }

    public void render(@NonNull PcListUiState state) {
        list.removeAllViews();
        for (PcEntry entry : state.getEntries()) {
            list.addView(createCard(entry));
        }
        searching.setVisibility(Views.visibleIf(state.isSearching()));
        searchHint.setVisibility(Views.visibleIf(state.showsHint()));
        searchHint.setText(state.getHint());
        allowNetwork.setVisibility(Views.visibleIf(state.showsAllowNetwork()));
    }

    @NonNull
    private View createCard(@NonNull PcEntry entry) {
        View card = inflater.inflate(R.layout.item_pc, list, false);
        TextView name = card.findViewById(R.id.pc_name);
        TextView detail = card.findViewById(R.id.pc_detail);
        name.setText(entry.getTitle().resolve(resources));
        detail.setText(entry.getDetail().resolve(resources));
        card.setOnClickListener(v -> listener.onEntrySelected(entry));
        return card;
    }
}
