package com.gazeora.gazeora.feature.connect.domain.usecase;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;

import com.gazeora.gazeora.core.stream.domain.model.StreamProtocol;
import com.gazeora.gazeora.core.stream.domain.model.StreamTarget;
import com.gazeora.gazeora.feature.connect.domain.parser.AddressParser;

import org.junit.Test;

public class ParseStreamAddressUseCaseTest {
    private final ParseStreamAddressUseCase parse =
            new ParseStreamAddressUseCase(new AddressParser());

    @Test
    public void trimsAndUsesGazeorasPortByDefault() {
        assertEquals(new StreamTarget(StreamProtocol.HTTP, "10.0.0.5", 8554, null, null),
                parse.execute(StreamProtocol.HTTP, "  10.0.0.5 ").get());
    }

    @Test
    public void rejectsNonAddresses() {
        assertFalse(parse.execute(StreamProtocol.UDP, "   ").isPresent());
    }
}
