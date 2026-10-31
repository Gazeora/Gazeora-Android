// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.connect.presentation.components;

import android.app.Activity;
import android.content.res.Resources;
import android.text.Editable;
import android.view.inputmethod.EditorInfo;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.gazeora.gazeora.R;
import com.gazeora.gazeora.core.stream.domain.model.StreamProtocol;
import com.gazeora.gazeora.core.ui.StreamProtocolLabels;
import com.gazeora.gazeora.core.ui.TextEditedWatcher;
import com.gazeora.gazeora.core.ui.UiText;
import com.gazeora.gazeora.core.ui.UnfilteredArrayAdapter;
import com.gazeora.gazeora.core.ui.Views;
import com.gazeora.gazeora.feature.connect.presentation.ManualEntryUiState;
import com.google.android.material.textfield.MaterialAutoCompleteTextView;
import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;

/** The "connect manually" form: a protocol, and for the network ones the PC's address. */
public final class ManualEntryView {
    /** What the user does with the form. */
    public interface Listener {
        void onProtocolSelected(@NonNull StreamProtocol protocol);

        void onAddressEdited();

        void onWatch(@NonNull String address);
    }

    private final Resources resources;
    private final Listener listener;
    private final MaterialAutoCompleteTextView protocolField;
    private final TextInputLayout addressLayout;
    private final TextInputEditText address;

    public ManualEntryView(@NonNull Activity activity, @NonNull Listener listener) {
        resources = activity.getResources();
        this.listener = listener;
        protocolField = activity.findViewById(R.id.protocol);
        addressLayout = activity.findViewById(R.id.address_layout);
        address = activity.findViewById(R.id.address);
        bindProtocols(activity);
        bindAddress();
        activity.findViewById(R.id.watch).setOnClickListener(v -> watch());
    }

    public void setAddress(@NonNull String text) {
        address.setText(text);
    }

    public void render(@NonNull ManualEntryUiState state) {
        protocolField.setText(resources.getString(StreamProtocolLabels.of(state.getProtocol())),
                false);
        addressLayout.setVisibility(Views.visibleIf(state.showsAddress()));
        UiText error = state.getAddressError();
        addressLayout.setError(error == null ? null : error.resolve(resources));
    }

    private void bindProtocols(@NonNull Activity activity) {
        StreamProtocol[] protocols = StreamProtocol.values();
        String[] labels = new String[protocols.length];
        for (int i = 0; i < protocols.length; ++i) {
            labels[i] = resources.getString(StreamProtocolLabels.of(protocols[i]));
        }
        protocolField.setAdapter(new UnfilteredArrayAdapter(activity, R.layout.item_protocol,
                labels));
        protocolField.setOnItemClickListener((parent, view, position, id) ->
                listener.onProtocolSelected(protocols[position]));
    }

    private void bindAddress() {
        address.setOnEditorActionListener((v, actionId, event) -> {
            boolean go = actionId == EditorInfo.IME_ACTION_GO;
            if (go) {
                watch();
            }
            return go;
        });
        address.addTextChangedListener(new TextEditedWatcher(listener::onAddressEdited));
    }

    private void watch() {
        listener.onWatch(textOf(address.getText()));
    }

    @NonNull
    private static String textOf(@Nullable Editable text) {
        return text == null ? "" : text.toString();
    }
}
