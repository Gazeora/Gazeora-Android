// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

namespace gazeora {

// A hand the back camera saw, as the hand tracker (MediaPipe's hand landmarker) gives it: 21
// landmarks, from the wrist out along each finger.
struct HandLandmarks {
    static constexpr int kCount = 21;
    static constexpr int kWrist = 0;
    static constexpr int kIndexBase = 5;
    static constexpr int kMiddleBase = 9;
    static constexpr int kLittleBase = 17;

    // In the upright camera picture: 0..1 across (right) and down.
    float image[kCount][2] = {};
    // The hand's own 3D shape (MediaPipe's world landmarks): each landmark from the hand's
    // middle, in meters, x right, y down and z away, as the picture's.
    float shape[kCount][3] = {};
    bool right = true;  // the tracker's guess: the viewer's right hand, or left
};

// Where a landmark shows in an eye: the tangents of its angle off straight ahead, across
// (right) and down.
struct ViewTangent {
    float x = 0;
    float y = 0;
};

// A hand where the eyes show it.
struct ShownHand {
    ViewTangent points[HandLandmarks::kCount];
    // How much farther each landmark is than the wrist, in the wrist's distances.
    float depth[HandLandmarks::kCount] = {};
    bool right = true;
    // An adult's palm length (kPalmLength) as a tangent at the wrist: kPalmLength over the
    // wrist's distance, so the camera's units per meter there are palm / kPalmLength.
    float palm = 0;
    // Whether its depths and distance come from its 3D shape put where the picture shows it;
    // if not, they're guesses: the distance from how big the palm looks, and the depths its
    // shape has at that distance.
    bool solid = false;
};

namespace hand_geometry {

// An adult's palm, wrist to middle knuckle, its width across the knuckles, and the forearm,
// wrist to elbow, in meters.
constexpr float kPalmLength = 0.09f;
constexpr float kPalmWidth = 0.065f;
constexpr float kForearm = 0.25f;
// The farthest the viewer's hands go from the camera on their head, in meters.
constexpr float kArmsReach = 0.7f;
// The viewer's fingers point up, away or to the side, never back down at them by more than
// this (the sine of 30 degrees below level); other people's hands reaching out to the viewer
// do.
constexpr float kMostDown = 0.5f;

// The hand where the eyes see it, as the camera does: its picture spans `tanAcross` x
// `tanDown` (half its angles; camera_geometry::UprightTangents). Its 3D shape, put where each
// landmark lines up with where the picture shows it, gives how far it is and its depths.
ShownHand Show(const HandLandmarks& hand, float tanAcross, float tanDown);

// How right-handed the hand's 3D shape is (after Show, with its depths): the thumb and bent
// fingers lie to the palm's side of it, which is one way for a right hand and the other for a
// left. Positive for a right hand, negative for a left, about 0 for a flat hand or no depths;
// in palm lengths, about -1 to 1.
float Rightness(const ShownHand& hand);

// Near enough to be the viewer's: within arm's reach of the camera on their head.
bool WithinReach(const ShownHand& hand);

// Whether the hand reaches out from the viewer, for the camera that sees `tanAcross` x
// `tanDown`: its fingers don't point back down at them, and its arm comes from out of view,
// below or to the side, where their body is. Other people's hands in view show their arms,
// or point at the viewer.
bool ComesFromViewer(const ShownHand& hand, float tanAcross, float tanDown);

}  // namespace hand_geometry

}  // namespace gazeora
