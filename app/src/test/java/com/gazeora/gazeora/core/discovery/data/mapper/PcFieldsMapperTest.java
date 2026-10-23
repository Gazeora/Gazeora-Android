package com.gazeora.gazeora.core.discovery.data.mapper;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import com.gazeora.gazeora.core.discovery.domain.model.Pc;
import com.gazeora.gazeora.core.stream.domain.model.StreamProtocol;

import org.junit.Test;

import java.util.List;

public class PcFieldsMapperTest {
    private final PcFieldsMapper mapper = new PcFieldsMapper();

    @Test
    public void mapsSixFieldsAPc() {
        List<Pc> pcs = mapper.map(new String[] {
                "Desk", "192.168.1.20", "rtsp-tcp", "8554", "/live", "h265",
                "Den", "192.168.1.21", "usb", "0", "", "h264"});

        assertEquals(2, pcs.size());
        assertEquals(new Pc.Builder().name("Desk").host("192.168.1.20")
                .protocol(StreamProtocol.RTSP_TCP).port(8554).path("/live").codec("h265")
                .build(), pcs.get(0));
        assertEquals(StreamProtocol.USB, pcs.get(1).getProtocol());
    }

    @Test
    public void skipsUnknownProtocolsBadPortsAndLeftovers() {
        assertTrue(mapper.map(new String[] {
                "A", "h", "carrier-pigeon", "1", "", "h264",
                "B", "h", "udp", "not-a-port", "", "h264",
                "C", "h"}).isEmpty());
    }
}
