// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <functional>

#include "head/quaternion.h"

namespace gazeora {

// Where the phone points, as it changes.
class OrientationSource {
public:
    // The phone's axes turned into Android's sensor world (Z up). From the source's thread.
    using Listener = std::function<void(const Quaternion& device)>;

    virtual ~OrientationSource() = default;
    virtual void Start(Listener listener) = 0;
    // Once it returns, the listener gets no more calls.
    virtual void Stop() = 0;
};

}  // namespace gazeora
