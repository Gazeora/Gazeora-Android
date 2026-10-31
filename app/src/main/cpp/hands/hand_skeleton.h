#pragma once

#include "hands/hand_geometry.h"
#include "hands/vec3.h"

namespace gazeora {

// Square axes of a part of the hand: along it, toward the palm's side of it, and across it.
struct Axes {
    Vec3 along;
    Vec3 palmward;
    Vec3 across;
};

// A bone of the hand as the skeleton has it: from `start`, `length` along its axes; in meters,
// in the camera's space.
struct Bone {
    Vec3 start;
    Axes axes;
    float length = 0;

    Vec3 End() const { return start + axes.along * length; }
};

// The viewer's hand as a skeleton that can only do what a hand can: a palm of fixed size and
// fingers of fixed lengths, posed by joint angles within a hand's reach. Like a VR headset's,
// it's fitted to what the tracker saw, so the tracker's noise can bend it only as far as a
// hand bends: never stretch it, flip a finger back or twist it about itself.
struct HandSkeleton {
    static constexpr int kFingers = 5;  // the thumb first, then index to little
    static constexpr int kBones = 3;    // from the hand out

    Vec3 wrist;
    // along from the wrist toward the knuckles, palmward, across toward the thumb's side
    Axes palm;
    Bone bones[kFingers][kBones];
    bool right = true;
    // How far apart the tracker saw the thumb's and index finger's tips, in meters, their
    // depths counting less (hand_skeleton::Fit); below 0 where it's not known, and its own tips
    // say.
    float tipsApart = -1;
};

namespace hand_skeleton {

// The skeleton's palm, in meters: from the wrist to the middle knuckle; and each finger's
// knuckle, along the palm, across it toward the thumb's side, and palmward of its middle. The
// skeleton's measures are the rigged hand's (hand_model_data.h, from
// tools/hand_model/convert.py), so its skin fits its bones.
constexpr float kPalmLength = 0.09f;
constexpr float kKnuckles[HandSkeleton::kFingers][3] = {
    {0.0281f, 0.0323f, 0.0155f},  // the thumb's base, by the wrist
    {0.0898f, 0.0289f, 0.0058f},
    {0.0900f, 0.0f, 0.0f},
    {0.0858f, -0.0293f, 0.0015f},
    {0.0788f, -0.0528f, 0.0058f},
};
// Each bone's length, in meters, from the hand out.
constexpr float kLengths[HandSkeleton::kFingers][HandSkeleton::kBones] = {
    {0.0271f, 0.0364f, 0.0298f},
    {0.0417f, 0.0277f, 0.0206f},
    {0.0453f, 0.0356f, 0.0218f},
    {0.0396f, 0.0313f, 0.0195f},
    {0.0315f, 0.0236f, 0.0161f},
};

// How far each joint bends, in radians, toward the palm (negative bends back). A finger's
// knuckle, and how far it spreads either way from its rest; its middle joint; its tip's.
constexpr float kKnuckleBack = -0.4f;
constexpr float kKnuckleBend = 1.6f;
constexpr float kSpread = 0.3f;
constexpr float kMiddleBend = 1.9f;
constexpr float kTipBend = 1.5f;
// The thumb at rest: its first bone's way along, across and palmward of the palm; and the way
// it curls, toward the little finger. Both are the rigged hand's.
constexpr float kThumbRest[3] = {0.64f, 0.53f, 0.56f};
constexpr float kThumbCurl[3] = {0.64f, -0.77f, 0.0f};
// The thumb: how far its first bone turns from its rest, and how far its next two bend.
constexpr float kThumbTurn = 0.95f;
constexpr float kThumbMiddleBack = -0.2f;
constexpr float kThumbMiddleBend = 1.2f;
constexpr float kThumbTipBack = -0.3f;
constexpr float kThumbTipBend = 1.5f;

// The skeleton for a hand the tracker saw, with its depths (hand_geometry::Show): the palm at
// the wrist, turned as the tracker saw it, and each joint bent as near as a hand can to how
// the tracker saw it. In a pinch, as the tracker's thumb and index tips meet, so do the
// skeleton's.
HandSkeleton Fit(const ShownHand& hand);

// Moves `drawn` toward `fitted` (the same hand, fitted later) for `seconds`: its wrist, palm
// and each bone turning toward where they're going, slowly while they're near it, so the
// tracker's wobble is smoothed away, and quickly once they're far, so its moves aren't lagged.
// Each bone keeps its length and starts where the one before ends, so on the way it's always a
// hand. A hand changing sides takes `fitted` at once.
void Glide(HandSkeleton& drawn, const HandSkeleton& fitted, float seconds);

// The axes the skeleton gives a bone pointing `along` (a unit vector): a finger's first bone,
// turned from the palm's `palm`; the thumb's first, curling across the palm; and each next
// bone, bent from the one before, `before`.
Axes FingerAxes(const Axes& palm, Vec3 along);
Axes ThumbAxes(const Axes& palm, Vec3 along);
Axes NextAxes(const Axes& before, Vec3 along);

}  // namespace hand_skeleton

}  // namespace gazeora
