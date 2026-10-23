package com.gazeora.gazeora.feature.player.data.mapper;

import static org.junit.Assert.assertEquals;

import com.gazeora.gazeora.feature.player.domain.model.PlayerStatus;

import org.junit.Test;

public class PlayerStatusMapperTest {
    private final PlayerStatusMapper mapper = new PlayerStatusMapper();

    @Test
    public void mapsTheNativeCodes() {
        assertEquals(PlayerStatus.CONNECTING, mapper.map(0));
        assertEquals(PlayerStatus.PLAYING, mapper.map(1));
        assertEquals(PlayerStatus.NO_FRAMES, mapper.map(2));
        assertEquals(PlayerStatus.FAILED, mapper.map(3));
        assertEquals(PlayerStatus.RTSP_UDP_BLOCKED, mapper.map(4));
        assertEquals(PlayerStatus.USB_CLOSED, mapper.map(5));
    }

    @Test
    public void unknownCodesReadAsConnecting() {
        assertEquals(PlayerStatus.CONNECTING, mapper.map(-1));
        assertEquals(PlayerStatus.CONNECTING, mapper.map(99));
    }
}
