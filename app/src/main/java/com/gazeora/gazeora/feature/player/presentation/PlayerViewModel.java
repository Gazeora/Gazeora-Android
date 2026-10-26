package com.gazeora.gazeora.feature.player.presentation;

import android.view.Surface;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.SavedStateHandle;
import androidx.lifecycle.ViewModel;

import com.gazeora.gazeora.core.discovery.domain.model.Pc;
import com.gazeora.gazeora.core.discovery.domain.repository.DiscoveryRepository;
import com.gazeora.gazeora.core.network.domain.LocalNetworkAccess;
import com.gazeora.gazeora.core.stream.domain.model.StreamTarget;
import com.gazeora.gazeora.core.usb.domain.UsbConnection;
import com.gazeora.gazeora.core.usb.domain.UsbStatus;
import com.gazeora.gazeora.feature.player.domain.model.KnownUsbPcs;
import com.gazeora.gazeora.feature.player.domain.model.PlayerStatus;
import com.gazeora.gazeora.feature.player.domain.model.ScreenGeometry;
import com.gazeora.gazeora.feature.player.domain.model.StreamPicture;
import com.gazeora.gazeora.feature.player.domain.player.PlaybackListener;
import com.gazeora.gazeora.feature.player.domain.touch.TouchGestureInterpreter;
import com.gazeora.gazeora.feature.player.domain.touch.TouchSample;
import com.gazeora.gazeora.feature.player.domain.usecase.FindMovedPcUseCase;
import com.gazeora.gazeora.feature.player.presentation.navigation.StreamTargetArgs;

import java.util.List;

import javax.inject.Inject;

import dagger.hilt.android.lifecycle.HiltViewModel;

/**
 * Plays the PC's stream while the screen is in sight, and follows the PC when it switches
 * protocol. While the picture is flat (not VR), touches on it drive the PC's mouse.
 */
@HiltViewModel
public final class PlayerViewModel extends ViewModel implements PlaybackListener {
    private static final float UNKNOWN_ASPECT_RATIO = 0f;

    private final MutableLiveData<PlayerUiState> uiState = new MutableLiveData<>();
    private final MutableLiveData<Float> aspectRatio = new MutableLiveData<>(UNKNOWN_ASPECT_RATIO);
    private final MutableLiveData<StreamPicture> picture =
            new MutableLiveData<>(StreamPicture.NONE);
    private final KnownUsbPcs knownUsbPcs = new KnownUsbPcs();
    private final PlaybackController playback;
    private final DiscoveryRepository discovery;
    private final LocalNetworkAccess networkAccess;
    private final UsbConnection usb;
    private final FindMovedPcUseCase findMovedPc;
    private final PlayerUiStateFactory states;
    private final TouchGestureInterpreter touch;
    private StreamTarget target;
    private boolean started;
    private boolean pictureShown;

    @Inject
    public PlayerViewModel(@NonNull SavedStateHandle args, @NonNull PlaybackController playback,
            @NonNull DiscoveryRepository discovery, @NonNull LocalNetworkAccess networkAccess,
            @NonNull UsbConnection usb, @NonNull FindMovedPcUseCase findMovedPc,
            @NonNull PlayerUiStateFactory states, @NonNull TouchGestureInterpreter touch) {
        this.target = StreamTargetArgs.read(args);
        this.playback = playback;
        this.discovery = discovery;
        this.networkAccess = networkAccess;
        this.usb = usb;
        this.findMovedPc = findMovedPc;
        this.states = states;
        this.touch = touch;
    }

    @NonNull
    public LiveData<PlayerUiState> getUiState() {
        return uiState;
    }

    @NonNull
    public LiveData<Float> getAspectRatio() {
        return aspectRatio;
    }

    /** The picture as shown, for placing touches on it. */
    @NonNull
    public LiveData<StreamPicture> getPicture() {
        return picture;
    }

    public void onStart() {
        started = true;
        play();
        if (networkAccess.isGranted()) {
            discovery.start(this::onPcsChanged);
        }
    }

    /** Stops streaming while out of sight; onStart reconnects. */
    public void onStop() {
        started = false;
        touch.cancel();
        playback.stop();
        discovery.stop();
    }

    public void onSurfaceCreated(@NonNull Surface surface) {
        playback.setSurface(surface);
        if (started) {
            startPlayback();
        }
    }

    public void onSurfaceDestroyed() {
        touch.cancel();
        playback.stop();
        playback.setSurface(null);
    }

    public void onRetry() {
        play();
    }

    /** The screen turned over, or was first measured. */
    public void onScreenChanged(@NonNull ScreenGeometry screen) {
        playback.setScreen(screen);
    }

    public void onTouch(@NonNull TouchSample sample) {
        if (acceptsTouch()) {
            touch.onTouch(sample);
        } else {
            touch.cancel();
        }
    }

    @Override
    public void onStatus(@NonNull PlayerStatus status, @Nullable String detail) {
        if (status == PlayerStatus.USB_CLOSED) {
            pictureShown = false;
        }
        uiState.setValue(states.forStatus(status, snapshot(), detail));
    }

    @Override
    public void onVideoSize(int width, int height) {
        aspectRatio.setValue(height == 0 ? UNKNOWN_ASPECT_RATIO : (float) width / height);
    }

    @Override
    public void onFirstFrame() {
        pictureShown = true;
        uiState.setValue(states.playing());
    }

    @Override
    public void onPicture(@NonNull StreamPicture shown) {
        picture.setValue(shown);
        if (!acceptsTouch()) {
            touch.cancel();
        }
    }

    @Override
    protected void onCleared() {
        playback.stop();
        discovery.stop();
    }

    private void onPcsChanged(@NonNull List<Pc> pcs) {
        findMovedPc.execute(target, pcs, knownUsbPcs).ifPresent(this::follow);
    }

    private void onUsbStatus(@NonNull UsbStatus status) {
        states.forUsb(status, snapshot()).ifPresent(uiState::setValue);
    }

    private void follow(@NonNull StreamTarget moved) {
        target = moved;
        play();
    }

    private void play() {
        touch.cancel();
        playback.stop();
        uiState.setValue(states.connecting(snapshot()));
        startPlayback();
    }

    private void startPlayback() {
        if (playback.start(target, this, this::onUsbStatus)) {
            pictureShown = false;
            picture.setValue(StreamPicture.NONE);  // the new stream reports its own
        }
    }

    private boolean acceptsTouch() {
        StreamPicture shown = picture.getValue();
        return pictureShown && shown != null && shown.acceptsTouch();
    }

    @NonNull
    private PlaybackSnapshot snapshot() {
        return new PlaybackSnapshot(target, pictureShown, usb.isOpen());
    }
}
