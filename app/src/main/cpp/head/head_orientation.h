// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include "head/quaternion.h"

namespace gazeora {

// The viewer's head, from the phone in front of the eyes: the screen's axes are the head's
// (X right, Y up, the eyes looking into it down -Z), in a world with Y up, as HeadPose has it.
class HeadOrientation {
public:
    // `device` turns the phone's axes into Android's sensor world (Z up), as a rotation
    // vector sensor reports it; `quarterTurns` is the display's rotation (Surface.ROTATION_*).
    static Quaternion FromDevice(const Quaternion& device, int quarterTurns);
};

}  // namespace gazeora
