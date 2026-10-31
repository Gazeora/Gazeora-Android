// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

package com.gazeora.gazeora.feature.connect.presentation;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;

import com.gazeora.gazeora.R;
import com.gazeora.gazeora.core.discovery.domain.model.Pc;
import com.gazeora.gazeora.core.discovery.domain.repository.DiscoveryRepository;
import com.gazeora.gazeora.core.discovery.domain.repository.PcsListener;
import com.gazeora.gazeora.core.network.domain.LocalNetworkAccess;
import com.gazeora.gazeora.core.stream.domain.model.StreamProtocol;
import com.gazeora.gazeora.core.stream.domain.model.StreamTarget;
import com.gazeora.gazeora.core.ui.EventQueue;
import com.gazeora.gazeora.core.ui.UiText;
import com.gazeora.gazeora.core.usb.domain.UsbConnection;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Optional;

import javax.inject.Inject;

import dagger.hilt.android.lifecycle.HiltViewModel;

/** The home screen: the PCs found by themselves (and the one on the cable), or one typed in. */
@HiltViewModel
public final class ConnectViewModel extends ViewModel implements PcsListener {
    private final MutableLiveData<PcListUiState> pcList = new MutableLiveData<>();
    private final MutableLiveData<ManualEntryUiState> manualEntry = new MutableLiveData<>();
    private final EventQueue<ConnectEvent> events = new EventQueue<>();
    private final DiscoveryRepository discovery;
    private final UsbConnection usb;
    private final LocalNetworkAccess networkAccess;
    private final ConnectUseCases useCases;
    private final PcEntryFactory entries;
    private List<Pc> pcs = Collections.emptyList();
    private boolean askedForNetwork;
    @Nullable private StreamTarget waitingForPermission;

    @Inject
    public ConnectViewModel(@NonNull DiscoveryRepository discovery, @NonNull UsbConnection usb,
            @NonNull LocalNetworkAccess networkAccess, @NonNull ConnectUseCases useCases,
            @NonNull PcEntryFactory entries) {
        this.discovery = discovery;
        this.usb = usb;
        this.networkAccess = networkAccess;
        this.useCases = useCases;
        this.entries = entries;
        manualEntry.setValue(new ManualEntryUiState(useCases.initialProtocol().execute(), null));
    }

    @NonNull
    public LiveData<PcListUiState> getPcList() {
        return pcList;
    }

    @NonNull
    public LiveData<ManualEntryUiState> getManualEntry() {
        return manualEntry;
    }

    @NonNull
    public LiveData<List<ConnectEvent>> getEvents() {
        return events.asLiveData();
    }

    @NonNull
    public String getInitialAddress() {
        return useCases.initialAddress().execute();
    }

    public void onStart() {
        if (networkAccess.isGranted()) {
            discovery.start(this);
        } else if (!askedForNetwork) {
            askedForNetwork = true;
            events.post(new RequestNetworkPermissionEvent());
        }
        publishPcs();
    }

    public void onStop() {
        discovery.stop();
    }

    @Override
    public void onPcsChanged(@NonNull List<Pc> found) {
        pcs = found;
        publishPcs();
    }

    public void onEventsHandled(@NonNull List<ConnectEvent> handled) {
        events.markHandled(handled);
    }

    public void onEntrySelected(@NonNull PcEntry entry) {
        events.post(new OpenPlayerEvent(entry.getTarget()));
    }

    /** The PC switched this phone to USB accessory mode: it wants the stream at once. */
    public void onUsbAccessoryAttached() {
        events.post(new OpenPlayerEvent(StreamTarget.usb()));
    }

    public void onProtocolSelected(@NonNull StreamProtocol protocol) {
        manualEntry.setValue(currentEntry().withProtocol(protocol));
    }

    public void onAddressEdited() {
        if (currentEntry().getAddressError() != null) {
            manualEntry.setValue(currentEntry().withAddressError(null));
        }
    }

    public void onWatch(@NonNull String address) {
        StreamProtocol protocol = currentEntry().getProtocol();
        useCases.saveConnection().execute(protocol);
        if (!protocol.needsAddress()) {
            events.post(new OpenPlayerEvent(StreamTarget.usb()));
            return;
        }
        Optional<StreamTarget> target = useCases.parseAddress().execute(protocol, address);
        if (!target.isPresent()) {
            showAddressError(R.string.connect_address_invalid);
            return;
        }
        useCases.saveConnection().execute(protocol, address);
        openWhenPermitted(target.get());
    }

    public void onNetworkPermissionResult(boolean granted) {
        StreamTarget waiting = waitingForPermission;
        waitingForPermission = null;
        if (granted) {
            discovery.start(this);
            if (waiting != null) {
                events.post(new OpenPlayerEvent(waiting));
            }
        } else if (waiting != null) {
            showAddressError(R.string.connect_local_network_denied);
        }
        publishPcs();
    }

    @Override
    protected void onCleared() {
        discovery.stop();
    }

    private void openWhenPermitted(@NonNull StreamTarget target) {
        if (useCases.needsPermission().execute(target)) {
            waitingForPermission = target;
            events.post(new RequestNetworkPermissionEvent());
        } else {
            events.post(new OpenPlayerEvent(target));
        }
    }

    private void publishPcs() {
        List<PcEntry> shown = new ArrayList<>();
        if (usb.isAvailable()) {
            shown.add(entries.usbCable());
        }
        for (Pc pc : pcs) {
            shown.add(entries.forPc(pc));
        }
        pcList.setValue(new PcListUiState(shown, networkAccess.isGranted()));
    }

    private void showAddressError(@StringRes int message) {
        manualEntry.setValue(currentEntry().withAddressError(UiText.of(message)));
    }

    @NonNull
    private ManualEntryUiState currentEntry() {
        ManualEntryUiState state = manualEntry.getValue();
        if (state == null) {
            throw new IllegalStateException("Manual entry state is set in the constructor");
        }
        return state;
    }
}
