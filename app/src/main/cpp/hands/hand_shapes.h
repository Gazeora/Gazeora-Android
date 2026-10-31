// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <bitset>
#include <vector>

#include "hands/hand_skeleton.h"

namespace gazeora {

// The shapes a hand makes that mean something, told apart as Handy.js (Stewart Smith's
// stewdio/vr-hands, MIT) tells them: by how far its joints are from each other.
enum class HandShape {
    kPinch,  // the thumb's tip on the index finger's
    kPoint,  // the index finger out, the rest curled, the thumb on the middle finger
    kFist,   // every finger curled, the thumb over them
    kL,      // the thumb and the index finger out, the rest curled
    kDevil,  // the index and little fingers out, the middle two held by the thumb
    kCount,
};

using HandShapes = std::bitset<size_t(HandShape::kCount)>;

namespace hand_shapes {

const char* Name(HandShape shape);

// The shapes `hand` makes now.
HandShapes Read(const HandSkeleton& hand);

// How near the thumb's and index finger's tips are to pinching: 0 apart, rising as they near
// each other, 1 once they pinch.
float Pinching(const HandSkeleton& hand);

}  // namespace hand_shapes

// A shape that a hand began or stopped making.
struct HandShapeChange {
    bool right = true;
    HandShape shape = HandShape::kPinch;
    bool began = true;
};

// Watches the viewer's hands' shapes, each hand (one left, one right) at most so often, as
// Handy does: a hand's shape holds for a moment, and checking it every frame is wasted.
class HandShapeWatcher {
public:
    // Each hand's shapes now: the changes since the last time each was checked.
    std::vector<HandShapeChange> Update(const std::vector<HandSkeleton>& hands, double now);
    void Clear();

private:
    struct Watched {
        HandShapes shapes;
        double checkedAt = -1e9;
    };
    Watched m_hands[2];  // left, right
};

}  // namespace gazeora
