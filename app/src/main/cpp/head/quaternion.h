// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

namespace gazeora {

// A rotation, as a unit quaternion.
struct Quaternion {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double w = 1.0;

    // `radians` about the unit axis (ax, ay, az), counterclockwise looking down the axis.
    static Quaternion AboutAxis(double ax, double ay, double az, double radians);

    // This rotation after `first`.
    Quaternion operator*(const Quaternion& first) const;
};

}  // namespace gazeora
