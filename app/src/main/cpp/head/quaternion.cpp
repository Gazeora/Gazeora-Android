// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "head/quaternion.h"

#include <cmath>

namespace gazeora {

Quaternion Quaternion::AboutAxis(double ax, double ay, double az, double radians) {
    const double half = radians / 2.0;
    const double s = std::sin(half);
    return Quaternion{ax * s, ay * s, az * s, std::cos(half)};
}

Quaternion Quaternion::operator*(const Quaternion& first) const {
    const Quaternion& a = *this;
    const Quaternion& b = first;
    return Quaternion{
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
    };
}

}  // namespace gazeora
