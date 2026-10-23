package com.gazeora.gazeora.feature.player.presentation;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.gazeora.gazeora.R;
import com.gazeora.gazeora.core.stream.domain.model.StreamProtocol;
import com.gazeora.gazeora.core.stream.domain.model.StreamTarget;
import com.gazeora.gazeora.core.ui.UiText;
import com.gazeora.gazeora.core.usb.domain.UsbStatus;
import com.gazeora.gazeora.feature.player.domain.model.PlayerStatus;

import java.util.EnumMap;
import java.util.Map;
import java.util.Optional;

import javax.inject.Inject;

/** Factory pattern: the player state for each status, picked by strategy. */
public final class PlayerUiStateFactory {
    private final Map<PlayerStatus, StatusUiStrategy> byStatus =
            new EnumMap<>(PlayerStatus.class);

    @Inject
    public PlayerUiStateFactory() {
        byStatus.put(PlayerStatus.CONNECTING, (snapshot, detail) -> connecting(snapshot));
        byStatus.put(PlayerStatus.PLAYING, (snapshot, detail) ->
                snapshot.isPictureShown() ? playing() : connecting(snapshot));
        byStatus.put(PlayerStatus.NO_FRAMES, (snapshot, detail) -> noFrames(snapshot));
        byStatus.put(PlayerStatus.FAILED, (snapshot, detail) ->
                failed(snapshot, detail == null ? null : UiText.of(detail)));
        byStatus.put(PlayerStatus.RTSP_UDP_BLOCKED, (snapshot, detail) ->
                failed(snapshot, UiText.of(R.string.player_rtsp_udp_blocked)));
        byStatus.put(PlayerStatus.USB_CLOSED, (snapshot, detail) ->
                new ProgressUiState(UiText.of(R.string.player_usb_waiting)));
    }

    @NonNull
    public PlayerUiState forStatus(@NonNull PlayerStatus status,
            @NonNull PlaybackSnapshot snapshot, @Nullable String detail) {
        StatusUiStrategy strategy = byStatus.get(status);
        return strategy == null ? connecting(snapshot) : strategy.create(snapshot, detail);
    }

    /** Empty when there's nothing new to show. */
    @NonNull
    public Optional<PlayerUiState> forUsb(@NonNull UsbStatus status,
            @NonNull PlaybackSnapshot snapshot) {
        Optional<Integer> message = UsbStatusMessages.of(status);
        if (message.isPresent()) {
            return Optional.of(new ProgressUiState(UiText.of(message.get())));
        }
        return snapshot.isPictureShown() ? Optional.empty() : Optional.of(connecting(snapshot));
    }

    @NonNull
    public PlayerUiState playing() {
        return new PlayingUiState();
    }

    @NonNull
    public PlayerUiState connecting(@NonNull PlaybackSnapshot snapshot) {
        StreamTarget target = snapshot.getTarget();
        boolean waitingForUsb = target.getProtocol() == StreamProtocol.USB && !snapshot.isUsbOpen();
        return new ProgressUiState(waitingForUsb
                ? UiText.of(R.string.player_usb_waiting)
                : UiText.of(R.string.player_connecting, TargetDisplayName.of(target)));
    }

    @NonNull
    private PlayerUiState noFrames(@NonNull PlaybackSnapshot snapshot) {
        StreamTarget target = snapshot.getTarget();
        return new ProgressUiState(target.getProtocol() == StreamProtocol.UDP
                ? UiText.of(R.string.player_no_frames_udp, target.getHost(), target.getPort())
                : UiText.of(R.string.player_no_frames, TargetDisplayName.of(target)));
    }

    @NonNull
    private PlayerUiState failed(@NonNull PlaybackSnapshot snapshot, @Nullable UiText detail) {
        return new FailedUiState(
                UiText.of(R.string.player_error, TargetDisplayName.of(snapshot.getTarget())),
                detail);
    }
}
