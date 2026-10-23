package com.gazeora.gazeora.feature.connect.domain.usecase;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import com.gazeora.gazeora.core.stream.domain.model.StreamProtocol;
import com.gazeora.gazeora.core.stream.domain.model.StreamTarget;

import org.junit.Test;

public class NeedsLocalNetworkPermissionUseCaseTest {
    private static final StreamTarget LAN =
            new StreamTarget(StreamProtocol.RTSP_TCP, "192.168.1.20", 8554, null, null);
    private static final StreamTarget LOOPBACK =
            new StreamTarget(StreamProtocol.RTSP_TCP, "127.0.0.1", 8554, null, null);

    @Test
    public void needsPermissionForTheLanUntilGranted() {
        assertTrue(new NeedsLocalNetworkPermissionUseCase(() -> false).execute(LAN));
        assertFalse(new NeedsLocalNetworkPermissionUseCase(() -> true).execute(LAN));
    }

    @Test
    public void loopbackNeverNeedsIt() {
        assertFalse(new NeedsLocalNetworkPermissionUseCase(() -> false).execute(LOOPBACK));
    }
}
