// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "head/head_orientation.h"

namespace gazeora {

namespace {

constexpr double kQuarterTurn = 1.57079632679489661923;

// The display turned by ROTATION_90 is the phone turned a quarter counterclockwise, so the
// screen's axes are the phone's turned a quarter clockwise about its Z.
Quaternion ScreenInDevice(int quarterTurns) {
    return Quaternion::AboutAxis(0.0, 0.0, 1.0, -quarterTurns * kQuarterTurn);
}

// Android's world has Z up; the head's has Y up: (x, y, z) there is (x, z, -y) here.
Quaternion YUpFromZUp() { return Quaternion::AboutAxis(1.0, 0.0, 0.0, -kQuarterTurn); }

}  // namespace

Quaternion HeadOrientation::FromDevice(const Quaternion& device, int quarterTurns) {
    return YUpFromZUp() * device * ScreenInDevice(quarterTurns);
}

}  // namespace gazeora
