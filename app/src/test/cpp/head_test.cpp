// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include <gtest/gtest.h>

#include <cmath>

#include "head/head_orientation.h"
#include "head/quaternion.h"

namespace gazeora {
namespace {

constexpr double kTolerance = 1e-9;
constexpr int kRotation90 = 1;
constexpr int kRotation270 = 3;

struct Vec {
    double x, y, z;
};

Vec Rotate(const Quaternion& q, const Vec& v) {
    Quaternion conjugate{-q.x, -q.y, -q.z, q.w};
    Quaternion turned = q * Quaternion{v.x, v.y, v.z, 0.0} * conjugate;
    return {turned.x, turned.y, turned.z};
}

void ExpectNear(const Vec& actual, const Vec& expected) {
    EXPECT_NEAR(actual.x, expected.x, kTolerance);
    EXPECT_NEAR(actual.y, expected.y, kTolerance);
    EXPECT_NEAR(actual.z, expected.z, kTolerance);
}

// The rotation whose columns are where the phone's X, Y and Z point in the world.
Quaternion FromAxes(const Vec& x, const Vec& y, const Vec& z) {
    const double trace = x.x + y.y + z.z;
    if (trace > 0.0) {
        const double s = std::sqrt(trace + 1.0) * 2.0;
        return {(y.z - z.y) / s, (z.x - x.z) / s, (x.y - y.x) / s, s / 4.0};
    }
    if (x.x > y.y && x.x > z.z) {
        const double s = std::sqrt(1.0 + x.x - y.y - z.z) * 2.0;
        return {s / 4.0, (y.x + x.y) / s, (z.x + x.z) / s, (y.z - z.y) / s};
    }
    if (y.y > z.z) {
        const double s = std::sqrt(1.0 + y.y - x.x - z.z) * 2.0;
        return {(y.x + x.y) / s, s / 4.0, (z.y + y.z) / s, (z.x - x.z) / s};
    }
    const double s = std::sqrt(1.0 + z.z - x.x - y.y) * 2.0;
    return {(z.x + x.z) / s, (z.y + y.z) / s, s / 4.0, (x.y - y.x) / s};
}

// Android's sensor world: X east, Y north, Z up. The head's world: X east, Y up, Z south.
const Vec kUp{0, 0, 1};
const Vec kDown{0, 0, -1};
const Vec kSouth{0, -1, 0};
const Vec kEast{1, 0, 0};
const Vec kWest{-1, 0, 0};

const Vec kForward{0, 0, -1};
const Vec kHeadUp{0, 1, 0};

TEST(QuaternionTest, ComposesRightToLeft) {
    const double quarter = std::acos(0.0);
    Quaternion aboutZ = Quaternion::AboutAxis(0, 0, 1, quarter);
    Quaternion aboutX = Quaternion::AboutAxis(1, 0, 0, quarter);
    // X about Z goes to Y; Y about X goes to Z.
    ExpectNear(Rotate(aboutX * aboutZ, {1, 0, 0}), {0, 0, 1});
}

TEST(HeadOrientationTest, LandscapeLeftLookingNorthIsStraightAhead) {
    // ROTATION_90: the phone's top to the left, its screen to the eyes.
    Quaternion device = FromAxes(kUp, kWest, kSouth);
    Quaternion head = HeadOrientation::FromDevice(device, kRotation90);
    ExpectNear(Rotate(head, kForward), kForward);
    ExpectNear(Rotate(head, kHeadUp), kHeadUp);
}

TEST(HeadOrientationTest, LandscapeRightLookingNorthIsStraightAhead) {
    // ROTATION_270: the phone's top to the right.
    Quaternion device = FromAxes(kDown, kEast, kSouth);
    Quaternion head = HeadOrientation::FromDevice(device, kRotation270);
    ExpectNear(Rotate(head, kForward), kForward);
    ExpectNear(Rotate(head, kHeadUp), kHeadUp);
}

TEST(HeadOrientationTest, FollowsTheHeadTurningLeft) {
    // Looking west: the screen faces east, the phone's top points south.
    Quaternion device = FromAxes(kUp, kSouth, kEast);
    Quaternion head = HeadOrientation::FromDevice(device, kRotation90);
    ExpectNear(Rotate(head, kForward), {-1, 0, 0});
    ExpectNear(Rotate(head, kHeadUp), kHeadUp);
}

TEST(HeadOrientationTest, FollowsTheHeadLookingUp) {
    // Looking straight up: the screen faces the floor, the phone's top points west and the
    // screen's up (the top of the head) south.
    Quaternion device = FromAxes(kSouth, kWest, kDown);
    Quaternion head = HeadOrientation::FromDevice(device, kRotation90);
    ExpectNear(Rotate(head, kForward), {0, 1, 0});
    ExpectNear(Rotate(head, kHeadUp), {0, 0, 1});  // the top of the head to the south
}

}  // namespace
}  // namespace gazeora
