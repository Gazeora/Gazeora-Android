// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <deque>
#include <vector>

#include "hands/hand_geometry.h"
#include "protocols/pointer_message.h"

namespace gazeora {

// The viewer's hand as a pointer on Gazeora's window, the way VR headsets let a hand point at
// a screen out of reach: the middle of its palm moves a cursor, the middle of the camera's view
// stretched over the whole window so its corners are in easy reach. A pinch clicks where the
// cursor was as it began; a pinch that moves scrolls, the content following the hand, as a
// finger does on a touch screen, so no button is ever left held. The cursor is steadied: it
// stays still through the hand's tremor and the tracker's wobble, but keeps up once the hand
// moves (a One Euro filter); within a small still zone it doesn't move at all, so a hand held
// still holds it still; and it glides from one answer to the next as the screen refreshes.
// A pinch is seen across the picture, where the tracker is surest. Times in seconds, from any
// one clock; the window from 0 to 1 across and down from its top left.
class HandPointer {
public:
    // Where to draw the cursor.
    struct Cursor {
        bool shown = false;
        float x = 0;
        float y = 0;
        float pinching = 0;  // 0, rising to 1 as a pinch nears
        bool pinched = false;
    };

    // The part of the camera's view (from 0 to 1 across and down) stretched over the window:
    // well inside its edges, as a palm nearer them takes the fingers out of view, and the
    // tracker loses the hand.
    static constexpr float kLeft = 0.28f;
    static constexpr float kRight = 0.72f;
    static constexpr float kTop = 0.22f;
    static constexpr float kBottom = 0.6f;
    // The thumb's and index finger's tips pinch closer than this, as a share of how big the
    // hand looks, and let go farther than this; a pinch starts to near from this.
    static constexpr float kPinchOn = 0.22f;
    static constexpr float kPinchOff = 0.35f;
    static constexpr float kPinchNears = 0.5f;

    // The tracker's answer at `now`: the hand to point with, or none, for the camera that sees
    // `tanAcross` x `tanDown`. What the PC should do goes into `events`.
    void Update(const ShownHand* hand, float tanAcross, float tanDown, double now,
                std::vector<PointerEvent>& events);
    // The cursor to draw at `now`.
    Cursor CursorAt(double now);
    void Clear();

private:
    struct Point {
        float x = 0;
        float y = 0;
    };
    struct Filter {
        Point value;
        Point speed;
        double at = 0;
    };
    enum class State { None, Pointing, Pinched, Scrolling };

    Point Steady(Point raw, double now);
    Point Earlier(double now, double before) const;

    State m_state = State::None;
    Filter m_filter;
    Point m_held;  // the cursor: m_filter's, but for the still zone
    // The steadied cursor's recent places, newest last, to click where a pinch began.
    std::deque<std::pair<double, Point>> m_history;
    Point m_pressedAt;     // where a pinch clicks
    Point m_scrolledFrom;  // where it last scrolled from
    float m_carryVertical = 0;
    float m_carryHorizontal = 0;
    float m_pinching = 0;
    // Gliding to m_filter's.
    Point m_drawn;
    double m_drawnAt = 0;
};

}  // namespace gazeora
