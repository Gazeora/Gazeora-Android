// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "hands/camera_status_reporter.h"

namespace gazeora {

void CameraStatusReporter::Report(const CameraReport& report, int64_t nowMs) {
    const bool changed = m_sentAtMs < 0 || report.state != m_lastState;
    if (!m_send || (!changed && nowMs - m_sentAtMs < kRepeatEveryMs)) {
        return;
    }
    m_send(report);
    m_lastState = report.state;
    m_sentAtMs = nowMs;
}

}  // namespace gazeora
