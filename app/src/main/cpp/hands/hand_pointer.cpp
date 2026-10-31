// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "hands/hand_pointer.h"

#include <algorithm>
#include <cmath>
#include <iterator>

namespace gazeora {

namespace {

constexpr float kTwoPi = 6.2831853f;
// The cursor's steadying (a One Euro filter): its cutoff while still, in Hz, and how much
// higher per window width a second it moves; and the cutoff for that speed.
constexpr float kStillCutoff = 0.4f;
constexpr float kCutoffPerSpeed = 1.5f;
constexpr float kSpeedCutoff = 1.0f;
// The still zone: the cursor stays put till the steadied hand is this far from it, as a share
// of the window's height, and then trails it by this much.
constexpr float kStillZone = 0.012f;
// The hand can point this far past the window's edges, as a share of it, so the cursor, still
// trailing it, reaches them.
constexpr float kPastEdge = 0.1f;
// How long the cursor drawn takes to glide most of the way to the newest, in seconds.
constexpr double kGlide = 0.05;
// A pinch clicks where the cursor was this long before it closed: closing it moves the hand a
// little.
constexpr double kPinchLead = 0.12;
constexpr double kHistory = 0.5;
// A pinch that moves farther than this, as a share of the window's height, scrolls.
constexpr float kScrollStarts = 0.04f;
// Scrolling: a move the window's height turns the wheel 20 notches, as a finger's swipe does.
constexpr int kWheelNotch = 120;
constexpr float kWheelPerWindow = 20.0f * kWheelNotch;
// The middle of the palm: the wrist and the fingers' knuckles. A pinch hardly moves it.
constexpr int kPalm[] = {0, 5, 9, 13, 17};
constexpr int kThumbTip = 4;
constexpr int kIndexTip = 8;

float Distance(const ViewTangent& a, const ViewTangent& b) {
    return std::hypot(a.x - b.x, a.y - b.y);
}

float Alpha(float cutoff, float dt) {
    return 1 / (1 + 1 / (kTwoPi * cutoff * dt));
}

int WholeNotches(float units) { return int(units / kWheelNotch) * kWheelNotch; }

PointerEvent Event(PointerAction action, float x, float y) {
    PointerEvent event;
    event.action = action;
    event.x = x;
    event.y = y;
    return event;
}

}  // namespace

void HandPointer::Update(const ShownHand* hand, float tanAcross, float tanDown, double now,
                         std::vector<PointerEvent>& events) {
    if (!hand || tanAcross <= 0 || tanDown <= 0) {
        Clear();  // a pinch the hand left in doesn't click
        return;
    }
    // Where it points: the palm's middle, in the camera's view, stretched over the window.
    ViewTangent palm;
    for (int i : kPalm) {
        palm.x += hand->points[i].x / std::size(kPalm);
        palm.y += hand->points[i].y / std::size(kPalm);
    }
    const float u = (palm.x / tanAcross + 1) / 2, v = (palm.y / tanDown + 1) / 2;
    const Point raw = {std::clamp((u - kLeft) / (kRight - kLeft), -kPastEdge, 1 + kPastEdge),
                       std::clamp((v - kTop) / (kBottom - kTop), -kPastEdge, 1 + kPastEdge)};
    // How near a pinch: the tips apart across the picture, for how big the hand looks there.
    const ViewTangent* p = hand->points;
    const float size = std::max(Distance(p[HandLandmarks::kWrist], p[HandLandmarks::kMiddleBase]),
                                Distance(p[HandLandmarks::kIndexBase], p[HandLandmarks::kLittleBase]) *
                                    hand_geometry::kPalmLength / hand_geometry::kPalmWidth);
    const float apart = size > 0 ? Distance(p[kThumbTip], p[kIndexTip]) / size : 1;
    m_pinching = std::clamp((kPinchNears - apart) / (kPinchNears - kPinchOn), 0.0f, 1.0f);

    if (m_state == State::None) {
        m_filter = {raw, {}, now};
        m_held = raw;
        m_drawn = raw;
        m_drawnAt = now;
        m_history.clear();
        m_state = State::Pointing;
    }
    const Point steady = Steady(raw, now);
    const float off = std::hypot(steady.x - m_held.x, steady.y - m_held.y);
    if (off > kStillZone) {
        const float share = 1 - kStillZone / off;
        m_held.x += (steady.x - m_held.x) * share;
        m_held.y += (steady.y - m_held.y) * share;
    }
    const Point cursor = {std::clamp(m_held.x, 0.0f, 1.0f), std::clamp(m_held.y, 0.0f, 1.0f)};
    m_history.emplace_back(now, cursor);
    while (!m_history.empty() && now - m_history.front().first > kHistory) {
        m_history.pop_front();
    }

    switch (m_state) {
        case State::Pointing:
            if (apart < kPinchOn) {
                m_pressedAt = Earlier(now, kPinchLead);
                m_scrolledFrom = steady;
                m_state = State::Pinched;
            } else {
                events.push_back(Event(PointerAction::Move, cursor.x, cursor.y));
            }
            break;
        case State::Pinched:
            if (apart > kPinchOff) {
                events.push_back(Event(PointerAction::Click, m_pressedAt.x, m_pressedAt.y));
                m_state = State::Pointing;
            } else if (std::hypot(steady.x - m_scrolledFrom.x, steady.y - m_scrolledFrom.y) >
                       kScrollStarts) {
                // The wheel turns what's under the pointer: where the pinch began.
                events.push_back(Event(PointerAction::Move, m_pressedAt.x, m_pressedAt.y));
                m_carryVertical = m_carryHorizontal = 0;
                m_state = State::Scrolling;
            }
            break;
        case State::Scrolling:
            if (apart > kPinchOff) {
                m_state = State::Pointing;
                break;
            }
            {
                // The hand moving up pulls the content up: the wheel turns down.
                const float vertical =
                    m_carryVertical + (steady.y - m_scrolledFrom.y) * kWheelPerWindow;
                const float horizontal =
                    m_carryHorizontal - (steady.x - m_scrolledFrom.x) * kWheelPerWindow;
                const int notchesVertical = WholeNotches(vertical);
                const int notchesHorizontal = WholeNotches(horizontal);
                if (notchesVertical != 0 || notchesHorizontal != 0) {
                    PointerEvent scroll = Event(PointerAction::Scroll, m_pressedAt.x, m_pressedAt.y);
                    scroll.wheelVertical = notchesVertical;
                    scroll.wheelHorizontal = notchesHorizontal;
                    events.push_back(scroll);
                }
                m_carryVertical = vertical - notchesVertical;
                m_carryHorizontal = horizontal - notchesHorizontal;
                m_scrolledFrom = steady;
            }
            break;
        case State::None:
            break;
    }
}

HandPointer::Cursor HandPointer::CursorAt(double now) {
    Cursor cursor;
    if (m_state == State::None) {
        return cursor;
    }
    // Held where a pinch clicks or scrolls; else gliding after the hand.
    const bool held = m_state != State::Pointing;
    const Point to = held ? m_pressedAt
                          : Point{std::clamp(m_held.x, 0.0f, 1.0f), std::clamp(m_held.y, 0.0f, 1.0f)};
    const float share = float(1 - std::exp(-std::max(now - m_drawnAt, 0.0) / kGlide));
    m_drawn = held ? to : Point{m_drawn.x + (to.x - m_drawn.x) * share,
                                m_drawn.y + (to.y - m_drawn.y) * share};
    m_drawnAt = now;
    cursor.shown = true;
    cursor.x = m_drawn.x;
    cursor.y = m_drawn.y;
    cursor.pinching = held ? 1.0f : m_pinching;
    cursor.pinched = held;
    return cursor;
}

void HandPointer::Clear() {
    m_state = State::None;
    m_history.clear();
    m_pinching = 0;
}

HandPointer::Point HandPointer::Steady(Point raw, double now) {
    const float dt = float(now - m_filter.at);
    if (dt <= 0) {
        return m_filter.value;
    }
    const Point speed = {(raw.x - m_filter.value.x) / dt, (raw.y - m_filter.value.y) / dt};
    const float speedAlpha = Alpha(kSpeedCutoff, dt);
    m_filter.speed.x += (speed.x - m_filter.speed.x) * speedAlpha;
    m_filter.speed.y += (speed.y - m_filter.speed.y) * speedAlpha;
    const float cutoff = kStillCutoff + kCutoffPerSpeed * std::hypot(m_filter.speed.x, m_filter.speed.y);
    const float alpha = Alpha(cutoff, dt);
    m_filter.value.x += (raw.x - m_filter.value.x) * alpha;
    m_filter.value.y += (raw.y - m_filter.value.y) * alpha;
    m_filter.at = now;
    return m_filter.value;
}

HandPointer::Point HandPointer::Earlier(double now, double before) const {
    for (auto it = m_history.rbegin(); it != m_history.rend(); ++it) {
        if (now - it->first >= before) return it->second;
    }
    return m_history.empty() ? m_held : m_history.front().second;
}

}  // namespace gazeora
