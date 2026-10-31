#include "hands/hand_shapes.h"

#include <algorithm>

namespace gazeora {

namespace hand_shapes {

namespace {

// Handy's measures, in meters, from the base of a finger (the thumb's middle joint) to its tip:
// out at least this far (the thumb this far), curled under this far. A tip touching another
// joint is under this far from it; the thumb's tip pinches the index finger's under this far.
// It's nearing a pinch from this far (not Handy's: a relaxed hand's tips are about that far
// apart), and all the way there at this.
constexpr float kExtended = 0.06f;
constexpr float kThumbExtended = 0.03f;
constexpr float kContracted = 0.05f;
constexpr float kTouching = 0.04f;
constexpr float kPinched = 0.03f;
constexpr float kPinchNears = 0.045f;
constexpr float kPinchNear = 0.025f;

enum Finger { kThumb, kIndex, kMiddle, kRing, kLittle };

Vec3 Tip(const HandSkeleton& hand, int finger) {
    return hand.bones[finger][HandSkeleton::kBones - 1].End();
}

// From the finger's base (its knuckle; the thumb's middle joint) to its tip.
float Reach(const HandSkeleton& hand, int finger) {
    const Vec3 base = hand.bones[finger][finger == kThumb ? 1 : 0].start;
    return Length(Tip(hand, finger) - base);
}

bool Extended(const HandSkeleton& hand, int finger) {
    return Reach(hand, finger) >= (finger == kThumb ? kThumbExtended : kExtended);
}

bool Contracted(const HandSkeleton& hand, int finger) {
    return Reach(hand, finger) < kContracted;
}

// How far apart the thumb's and index finger's tips are: as the tracker saw them, where
// that's known.
float TipsApart(const HandSkeleton& hand) {
    return hand.tipsApart >= 0 ? hand.tipsApart : Length(Tip(hand, kThumb) - Tip(hand, kIndex));
}

// The thumb's tip on a finger's middle joint (bone 1) or its last (bone 2).
bool ThumbOn(const HandSkeleton& hand, int finger, int bone) {
    return Length(Tip(hand, kThumb) - hand.bones[finger][bone].start) < kTouching;
}

}  // namespace

const char* Name(HandShape shape) {
    switch (shape) {
        case HandShape::kPinch: return "pinch";
        case HandShape::kPoint: return "point";
        case HandShape::kFist: return "fist";
        case HandShape::kL: return "L";
        case HandShape::kDevil: return "devil horns";
        case HandShape::kCount: break;
    }
    return "?";
}

HandShapes Read(const HandSkeleton& hand) {
    const bool index = Extended(hand, kIndex), little = Extended(hand, kLittle);
    const bool indexIn = Contracted(hand, kIndex), middleIn = Contracted(hand, kMiddle);
    const bool ringIn = Contracted(hand, kRing), littleIn = Contracted(hand, kLittle);
    HandShapes shapes;
    shapes[size_t(HandShape::kPinch)] = TipsApart(hand) < kPinched;
    shapes[size_t(HandShape::kPoint)] =
        index && middleIn && ringIn && littleIn && ThumbOn(hand, kMiddle, 1);
    shapes[size_t(HandShape::kFist)] =
        indexIn && middleIn && ringIn && littleIn && ThumbOn(hand, kRing, 2);
    shapes[size_t(HandShape::kL)] =
        Extended(hand, kThumb) && index && middleIn && ringIn && littleIn;
    shapes[size_t(HandShape::kDevil)] =
        index && middleIn && ringIn && little && ThumbOn(hand, kRing, 2);
    return shapes;
}

float Pinching(const HandSkeleton& hand) {
    return std::clamp((kPinchNears - TipsApart(hand)) / (kPinchNears - kPinchNear), 0.0f, 1.0f);
}

}  // namespace hand_shapes

namespace {

// Handy's throttle, per hand, in seconds.
constexpr double kCheckEvery = 0.1;

}  // namespace

std::vector<HandShapeChange> HandShapeWatcher::Update(const std::vector<HandSkeleton>& hands,
                                                      double now) {
    std::vector<HandShapeChange> changes;
    bool seen[2] = {};
    for (const HandSkeleton& hand : hands) {
        Watched& watched = m_hands[hand.right];
        if (seen[hand.right]) continue;  // one of each
        seen[hand.right] = true;
        if (now - watched.checkedAt < kCheckEvery) continue;
        watched.checkedAt = now;
        const HandShapes shapes = hand_shapes::Read(hand);
        for (size_t s = 0; s < shapes.size(); ++s) {
            if (shapes[s] != watched.shapes[s]) {
                changes.push_back({hand.right, HandShape(s), shapes[s]});
            }
        }
        watched.shapes = shapes;
    }
    // A hand gone ends its shapes.
    for (int side = 0; side < 2; ++side) {
        if (seen[side] || m_hands[side].shapes.none()) continue;
        for (size_t s = 0; s < m_hands[side].shapes.size(); ++s) {
            if (m_hands[side].shapes[s]) changes.push_back({side == 1, HandShape(s), false});
        }
        m_hands[side] = Watched();
    }
    return changes;
}

void HandShapeWatcher::Clear() {
    m_hands[0] = m_hands[1] = Watched();
}

}  // namespace gazeora
