package com.gazeora.gazeora.feature.connect.presentation;

import androidx.annotation.NonNull;

import com.gazeora.gazeora.R;
import com.gazeora.gazeora.core.discovery.domain.model.Pc;
import com.gazeora.gazeora.core.stream.domain.model.StreamTarget;
import com.gazeora.gazeora.core.ui.StreamProtocolLabels;
import com.gazeora.gazeora.core.ui.UiText;

/** A PC streaming over the network. */
final class NetworkPcEntry extends PcEntry {
    private final Pc pc;

    NetworkPcEntry(@NonNull Pc pc) {
        this.pc = pc;
    }

    @NonNull
    @Override
    public UiText getTitle() {
        return UiText.of(pc.getName());
    }

    @NonNull
    @Override
    public UiText getDetail() {
        // The port as a plain string, so it keeps ASCII digits in every locale.
        return UiText.of(R.string.connect_pc_detail,
                UiText.of(StreamProtocolLabels.of(pc.getProtocol())), pc.getHost(),
                String.valueOf(pc.getPort()), CodecLabels.of(pc.getCodec()));
    }

    @NonNull
    @Override
    public StreamTarget getTarget() {
        return pc.toStreamTarget();
    }
}
