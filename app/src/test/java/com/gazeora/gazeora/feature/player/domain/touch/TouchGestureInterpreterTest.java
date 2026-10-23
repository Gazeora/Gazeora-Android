package com.gazeora.gazeora.feature.player.domain.touch;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import com.gazeora.gazeora.feature.player.domain.model.PointerAction;
import com.gazeora.gazeora.feature.player.domain.model.PointerButton;
import com.gazeora.gazeora.feature.player.domain.model.PointerCommand;

import org.junit.Test;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class TouchGestureInterpreterTest {
    private final List<PointerCommand> sent = new ArrayList<>();
    private final TouchGestureInterpreter touch = new TouchGestureInterpreter(sent::add);

    private static TouchSample at(TouchPhase phase, float x, float y) {
        return new TouchSample(phase, x, y, x, y);
    }

    @Test
    public void aTapIsOneClickWhereItLands() {
        touch.onTouch(at(TouchPhase.FIRST_DOWN, 0.2f, 0.3f));
        touch.onTouch(at(TouchPhase.LAST_UP, 0.2f, 0.3f));

        assertEquals(Collections.singletonList(
                PointerCommand.click(PointerButton.LEFT, 0.2f, 0.3f)), sent);
    }

    @Test
    public void oneFingerMovingScrollsFromWhereItLanded() {
        touch.onTouch(at(TouchPhase.FIRST_DOWN, 0.5f, 0.5f));
        // Up an eighth of the picture: 2.5 notches, the content follows, the wheel turns down.
        touch.onTouch(at(TouchPhase.MOVE, 0.5f, 0.375f));
        touch.onTouch(at(TouchPhase.LAST_UP, 0.5f, 0.375f));

        assertEquals(Arrays.asList(
                PointerCommand.move(0.5f, 0.5f),
                PointerCommand.scroll(-240, 0)), sent);
    }

    @Test
    public void smallMovesAddUpToANotch() {
        touch.onTouch(at(TouchPhase.FIRST_DOWN, 0.5f, 0.5f));
        for (int i = 1; i <= 4; i++) {
            touch.onTouch(at(TouchPhase.MOVE, 0.5f, 0.5f + i / 64f));  // 37.5 units each
        }

        assertEquals(PointerCommand.scroll(120, 0), sent.get(sent.size() - 1));
        assertEquals(2, sent.size());
    }

    @Test
    public void aWobbleIsStillATap() {
        touch.onTouch(at(TouchPhase.FIRST_DOWN, 0.5f, 0.5f));
        touch.onTouch(at(TouchPhase.MOVE, 0.505f, 0.495f));
        touch.onTouch(at(TouchPhase.LAST_UP, 0.505f, 0.495f));

        assertEquals(Collections.singletonList(
                PointerCommand.click(PointerButton.LEFT, 0.5f, 0.5f)), sent);
    }

    @Test
    public void twoFingersScrollToo() {
        touch.onTouch(at(TouchPhase.FIRST_DOWN, 0.5f, 0.5f));
        touch.onTouch(at(TouchPhase.EXTRA_DOWN, 0.5f, 0.5f));
        touch.onTouch(at(TouchPhase.MOVE, 0.5f, 0.625f));

        assertEquals(PointerCommand.scroll(240, 0), sent.get(sent.size() - 1));
    }

    @Test
    public void nothingIsEverLeftPressed() {
        touch.onTouch(at(TouchPhase.FIRST_DOWN, 0.5f, 0.5f));
        touch.onTouch(at(TouchPhase.MOVE, 0.5f, 0.9f));
        touch.cancel();
        touch.onTouch(at(TouchPhase.FIRST_DOWN, 0.1f, 0.1f));
        touch.onTouch(at(TouchPhase.CANCEL, 0.1f, 0.1f));

        for (PointerCommand command : sent) {
            assertTrue(command.getAction() != PointerAction.PRESS);
        }
    }
}
