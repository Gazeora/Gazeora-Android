// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <cstdint>
#include <functional>

#include "protocols/camera_status_message.h"

namespace gazeora {

// Passes the camera's status on when it changes, and again every so often, so the PC makes
// up for a report lost on the way.
class CameraStatusReporter {
public:
    using Send = std::function<void(const CameraReport& report)>;
    static constexpr int64_t kRepeatEveryMs = 1000;

    // `send` may be empty: nowhere to report to.
    explicit CameraStatusReporter(Send send) : m_send(std::move(send)) {}

    void Report(const CameraReport& report, int64_t nowMs);

private:
    Send m_send;
    CameraState m_lastState = CameraState::Off;
    int64_t m_sentAtMs = -1;
};

}  // namespace gazeora
