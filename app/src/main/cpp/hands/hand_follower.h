// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <vector>

#include "hands/hand_geometry.h"
#include "hands/hand_skeleton.h"

namespace gazeora {

// Follows the viewer's own hands from one hand tracker answer to the next, as VR headsets do.
// A hand the tracker finds becomes theirs only when it's within arm's reach and comes from
// them (hand_geometry::ComesFromViewer); from then on it's followed wherever it goes, until
// it's lost. Every other hand in view is left out, and once both of theirs are followed no
// other hand can take their place. The landmarks are smoothed: steady while the hand is
// still, with little lag as it moves. Each is fitted with the hand skeleton, which the hand
// drawn glides to as the screen refreshes (hand_skeleton::Glide), rather than jump to it, as
// the tracker answers less often. Whether each is the viewer's right or left hand is settled
// when it's found, mostly by where its arm comes in from, and kept: from behind, a right hand
// looks much like a left one from the front, which the tracker mixes up, and where it takes a
// hand for the other one, its 3D shape is turned back about. Times in seconds, from any one
// clock.
class HandFollower {
public:
    // At most the viewer's two hands.
    static constexpr int kMaxHands = 2;
    // How long a hand the tracker stops finding is still drawn and followed.
    static constexpr double kLostAfter = 0.3;

    // The tracker's answer at `now`: every hand it saw, the viewer's or not, for the camera
    // that sees `tanAcross` x `tanDown`.
    void Update(const std::vector<ShownHand>& seen, float tanAcross, float tanDown, double now);
    // The viewer's hands at `now`, as the tracker saw them, smoothed; nearest (biggest) first.
    std::vector<ShownHand> Hands(double now) const;
    // Their skeletons to draw at `now`, gliding to the newest fitted; the same order.
    std::vector<HandSkeleton> Skeletons(double now);
    void Clear() { m_followed.clear(); }

private:
    struct Followed {
        ShownHand hand;  // smoothed
        HandSkeleton fitted;  // to it
        HandSkeleton drawn;   // gliding to that
        double drawnAt = 0;
        double seenAt = 0;
    };

    std::vector<Followed> m_followed;
};

}  // namespace gazeora
