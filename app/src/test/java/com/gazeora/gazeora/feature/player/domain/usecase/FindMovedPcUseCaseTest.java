package com.gazeora.gazeora.feature.player.domain.usecase;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;

import com.gazeora.gazeora.core.discovery.domain.model.Pc;
import com.gazeora.gazeora.core.stream.domain.model.StreamProtocol;
import com.gazeora.gazeora.core.stream.domain.model.StreamTarget;
import com.gazeora.gazeora.feature.player.domain.model.KnownUsbPcs;

import org.junit.Test;

import java.util.Arrays;
import java.util.Collections;

public class FindMovedPcUseCaseTest {
    private static final String HOST = "192.168.1.20";

    private final FindMovedPcUseCase findMoved = new FindMovedPcUseCase();
    private final KnownUsbPcs known = new KnownUsbPcs();

    @Test
    public void followsTheSamePcToAnotherProtocol() {
        StreamTarget current = new StreamTarget(StreamProtocol.RTSP_TCP, HOST, 8554, null, "Desk");
        Pc moved = pc("Desk", StreamProtocol.HTTP, 8080);

        assertEquals(moved.toStreamTarget(),
                findMoved.execute(current, Collections.singletonList(moved), known).get());
    }

    @Test
    public void ignoresOtherPcsAndUnchangedOnes() {
        StreamTarget current = new StreamTarget(StreamProtocol.RTSP_TCP, HOST, 8554, null, "Desk");

        assertFalse(findMoved.execute(current, Arrays.asList(
                pc("Den", StreamProtocol.HTTP, 8080),
                pc("Desk", StreamProtocol.RTSP_TCP, 8554)), known).isPresent());
    }

    @Test
    public void matchesByAddressWhenTypedIn() {
        StreamTarget current = new StreamTarget(StreamProtocol.RTSP_TCP, HOST, 8554, null, null);

        assertEquals("Desk", findMoved.execute(current, Collections.singletonList(
                pc("Desk", StreamProtocol.UDP, 5000)), known).get().getName());
    }

    @Test
    public void movingToUsbKeepsTheAddressAndCanMoveBack() {
        StreamTarget current = new StreamTarget(StreamProtocol.RTSP_TCP, HOST, 8554, null, "Desk");
        StreamTarget onUsb = findMoved.execute(current, Collections.singletonList(
                pc("Desk", StreamProtocol.USB, 0)), known).get();

        assertEquals(current.switchedToUsb("Desk"), onUsb);
        Pc back = pc("Desk", StreamProtocol.RTSP_TCP, 8554);
        assertEquals(back.toStreamTarget(),
                findMoved.execute(onUsb, Collections.singletonList(back), known).get());
    }

    @Test
    public void onUsbOnlyFollowsPcsThatSaidUsb() {
        assertFalse(findMoved.execute(StreamTarget.usb(), Collections.singletonList(
                pc("Desk", StreamProtocol.RTSP_TCP, 8554)), known).isPresent());
    }

    private static Pc pc(String name, StreamProtocol protocol, int port) {
        return new Pc.Builder().name(name).host(HOST).protocol(protocol).port(port).build();
    }
}
