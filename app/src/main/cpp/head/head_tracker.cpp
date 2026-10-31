// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "head/head_tracker.h"

#include "head/head_orientation.h"

namespace gazeora {

HeadTracker::HeadTracker(std::unique_ptr<OrientationSource> source, HeadPoseSender sender,
                         int quarterTurns)
    : m_source(std::move(source)), m_sender(std::move(sender)), m_quarterTurns(quarterTurns) {}

HeadTracker::~HeadTracker() { m_source->Stop(); }

void HeadTracker::SetActive(bool active) {
    std::lock_guard<std::mutex> lock(m_activeMutex);
    if (active == m_active) {
        return;
    }
    m_active = active;
    if (active) {
        m_source->Start([this](const Quaternion& device) { OnOrientation(device); });
    } else {
        m_source->Stop();
    }
}

void HeadTracker::OnOrientation(const Quaternion& device) {
    m_sender.Send(HeadOrientation::FromDevice(device, m_quarterTurns));
}

}  // namespace gazeora
