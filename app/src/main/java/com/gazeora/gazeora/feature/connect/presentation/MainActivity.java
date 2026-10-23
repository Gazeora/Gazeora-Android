package com.gazeora.gazeora.feature.connect.presentation;

import android.content.Intent;
import android.hardware.usb.UsbManager;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.splashscreen.SplashScreen;
import androidx.lifecycle.ViewModelProvider;

import com.gazeora.gazeora.R;
import com.gazeora.gazeora.core.stream.domain.model.StreamProtocol;
import com.gazeora.gazeora.core.stream.domain.model.StreamTarget;
import com.gazeora.gazeora.feature.connect.presentation.components.ManualEntryView;
import com.gazeora.gazeora.feature.connect.presentation.components.NetworkPermissionRequester;
import com.gazeora.gazeora.feature.connect.presentation.components.PcListView;
import com.gazeora.gazeora.feature.connect.presentation.components.SplashController;
import com.gazeora.gazeora.feature.player.presentation.navigation.PlayerIntents;

import java.util.List;

import dagger.hilt.android.AndroidEntryPoint;

/**
 * The home screen. Opens behind the splash (Theme.Gazeora.Starting), or straight into the
 * stream when the PC switched this phone to USB accessory mode.
 */
@AndroidEntryPoint
public final class MainActivity extends AppCompatActivity implements ConnectEventHandler {
    private final NetworkPermissionRequester networkPermission =
            new NetworkPermissionRequester(this, this::onNetworkPermissionResult);
    private ConnectViewModel viewModel;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        SplashScreen splash = SplashScreen.installSplashScreen(this);
        super.onCreate(savedInstanceState);
        boolean forUsb = isAccessoryIntent(getIntent());
        new SplashController(splash).install(savedInstanceState == null && !forUsb);
        setContentView(R.layout.activity_main);
        viewModel = new ViewModelProvider(this).get(ConnectViewModel.class);
        bindViews(savedInstanceState == null);
        if (savedInstanceState == null && forUsb) {
            viewModel.onUsbAccessoryAttached();
        }
    }

    @Override
    protected void onNewIntent(@NonNull Intent intent) {
        super.onNewIntent(intent);
        if (isAccessoryIntent(intent)) {
            viewModel.onUsbAccessoryAttached();
        }
    }

    @Override
    protected void onStart() {
        super.onStart();
        viewModel.onStart();
    }

    @Override
    protected void onStop() {
        super.onStop();
        viewModel.onStop();
    }

    @Override
    public void openPlayer(@NonNull StreamTarget target) {
        startActivity(PlayerIntents.create(this, target));
    }

    @Override
    public void requestNetworkPermission() {
        networkPermission.request();
    }

    private void bindViews(boolean freshLaunch) {
        PcListView pcList = new PcListView(this, new PcListListener());
        ManualEntryView manualEntry = new ManualEntryView(this, new ManualEntryListener());
        if (freshLaunch) {
            manualEntry.setAddress(viewModel.getInitialAddress());
        }
        viewModel.getPcList().observe(this, pcList::render);
        viewModel.getManualEntry().observe(this, manualEntry::render);
        viewModel.getEvents().observe(this, this::handleEvents);
    }

    private void handleEvents(@NonNull List<ConnectEvent> events) {
        if (events.isEmpty()) {
            return;
        }
        for (ConnectEvent event : events) {
            event.dispatch(this);
        }
        viewModel.onEventsHandled(events);
    }

    private void onNetworkPermissionResult(@NonNull Boolean granted) {
        viewModel.onNetworkPermissionResult(granted);
    }

    private static boolean isAccessoryIntent(@Nullable Intent intent) {
        return intent != null
                && UsbManager.ACTION_USB_ACCESSORY_ATTACHED.equals(intent.getAction());
    }

    private final class PcListListener implements PcListView.Listener {
        @Override
        public void onEntrySelected(@NonNull PcEntry entry) {
            viewModel.onEntrySelected(entry);
        }

        @Override
        public void onAllowNetwork() {
            networkPermission.requestAgain();
        }
    }

    private final class ManualEntryListener implements ManualEntryView.Listener {
        @Override
        public void onProtocolSelected(@NonNull StreamProtocol protocol) {
            viewModel.onProtocolSelected(protocol);
        }

        @Override
        public void onAddressEdited() {
            viewModel.onAddressEdited();
        }

        @Override
        public void onWatch(@NonNull String address) {
            viewModel.onWatch(address);
        }
    }
}
