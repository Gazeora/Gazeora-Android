// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "hands/hand_overlay.h"

#include <android/log.h>

#include <chrono>
#include <cmath>

namespace gazeora {

namespace {

constexpr char kTag[] = "GazeoraHands";
// How often to say how the hands are seen, in seconds.
constexpr double kLogEvery = 1.0;

double NowSeconds() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

}  // namespace

void HandOverlay::Set(std::vector<HandLandmarks> hands) {
    m_seen = std::move(hands);
    m_seenNew = true;
    m_seenAt = NowSeconds();
    ++m_answers;
}

void HandOverlay::Clear() {
    m_seen.clear();
    m_seenNew = false;
    m_follower.Clear();
    m_pointer.Clear();
    m_pointerEvents.clear();
    m_pointing = false;
}

std::vector<HandSkeleton> HandOverlay::Hands(const CameraView& camera) {
    float across = 0, down = 0;
    camera_geometry::UprightTangents(camera, across, down);
    if (m_seenNew) {
        std::vector<ShownHand> seen;
        for (const HandLandmarks& hand : m_seen) {
            seen.push_back(hand_geometry::Show(hand, across, down));
        }
        m_follower.Update(seen, across, down, m_seenAt);
        m_seenNew = false;
        // The hand pointing keeps pointing while it's there; else the right hand points, or
        // the left without it.
        const std::vector<ShownHand> followed = m_follower.Hands(m_seenAt);
        const ShownHand* pointing = nullptr;
        float nearest = 0;
        for (const ShownHand& hand : followed) {
            const ViewTangent& wrist = hand.points[HandLandmarks::kWrist];
            const float apart = std::hypot(wrist.x - m_pointingAt.x, wrist.y - m_pointingAt.y);
            if (m_pointing && apart < hand.palm && (!pointing || apart < nearest)) {
                pointing = &hand;
                nearest = apart;
            }
        }
        if (!pointing) {
            for (const ShownHand& hand : followed) {
                if (!pointing || (hand.right && !pointing->right)) pointing = &hand;
            }
        }
        m_pointing = pointing != nullptr;
        if (pointing) m_pointingAt = pointing->points[HandLandmarks::kWrist];
        m_pointer.Update(pointing, across, down, m_seenAt, m_pointerEvents);
        if (m_seenAt - m_loggedAt >= kLogEvery) {
            const double perSecond = m_loggedAt > 0 ? m_answers / (m_seenAt - m_loggedAt) : 0;
            m_loggedAt = m_seenAt;
            m_answers = 0;
            if (seen.empty()) {
                __android_log_print(ANDROID_LOG_DEBUG, kTag, "%.1f answers a second; no hands",
                                    perSecond);
            } else {
                const ShownHand& hand = seen.front();
                __android_log_print(
                    ANDROID_LOG_DEBUG, kTag,
                    "%.1f answers a second; %zu hands; first %s, %.2f m off, rightness %+.2f "
                    "(tracker: %s)",
                    perSecond, seen.size(), hand.solid ? "in 3D" : "guessed in 3D",
                    hand_geometry::kPalmLength / hand.palm, hand_geometry::Rightness(hand),
                    hand.right ? "right" : "left");
            }
        }
    }
    return m_follower.Skeletons(NowSeconds());
}

std::vector<PointerEvent> HandOverlay::TakePointerEvents() {
    std::vector<PointerEvent> events;
    std::swap(events, m_pointerEvents);
    return events;
}

HandPointer::Cursor HandOverlay::Cursor() { return m_pointer.CursorAt(NowSeconds()); }

}  // namespace gazeora
