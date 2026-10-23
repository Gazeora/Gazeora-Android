package com.gazeora.gazeora.feature.connect.domain.parser;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;

import com.gazeora.gazeora.feature.connect.domain.model.HostAndPort;

import org.junit.Test;

public class AddressParserTest {
    private static final int DEFAULT_PORT = 8554;

    private final AddressParser parser = new AddressParser();

    @Test
    public void readsHostAndPort() {
        assertEquals(new HostAndPort("192.168.1.20", 9000),
                parser.parse("192.168.1.20:9000").get());
    }

    @Test
    public void hostAloneHasNoPort() {
        HostAndPort address = parser.parse("my-pc").get();
        assertEquals("my-pc", address.getHost());
        assertEquals(DEFAULT_PORT, address.getPortOr(DEFAULT_PORT));
    }

    @Test
    public void ignoresSchemePathAndUserInfo() {
        assertEquals(new HostAndPort("pc", 554),
                parser.parse("rtsp://user@pc:554/stream?x#y").get());
    }

    @Test
    public void badPortFallsBackToDefault() {
        assertEquals(DEFAULT_PORT, parser.parse("pc:abc").get().getPortOr(DEFAULT_PORT));
    }

    @Test
    public void rejectsEmptyAndIpv6() {
        assertFalse(parser.parse("").isPresent());
        assertFalse(parser.parse(":8554").isPresent());
        assertFalse(parser.parse("[::1]:8554").isPresent());
        assertFalse(parser.parse("a:b:c").isPresent());
    }
}
