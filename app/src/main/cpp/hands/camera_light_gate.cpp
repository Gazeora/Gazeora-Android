// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "hands/camera_light_gate.h"

namespace gazeora {

void CameraLightGate::Start(int64_t nowMs) {
    if (m_phase == Phase::Idle) {
        Enter(Phase::Watching, nowMs);
    }
}

void CameraLightGate::Stop() {
    m_phase = Phase::Idle;
    m_runningSinceMs = -1;
    m_darkSinceMs = -1;
    m_seen = {};
}

void CameraLightGate::Update(const CameraSighting& seen, int64_t nowMs) {
    m_seen = seen;
    if (!seen.running) {
        m_runningSinceMs = -1;
    } else if (m_runningSinceMs < 0) {
        m_runningSinceMs = nowMs;
    }
    switch (m_phase) {
        case Phase::Idle:
            break;
        case Phase::Watching:
            Watch(nowMs);
            break;
        case Phase::Resting:
            Rest(nowMs);
            break;
        case Phase::Checking:
            Check(nowMs);
            break;
    }
}

bool CameraLightGate::WantsCamera() const {
    return m_phase == Phase::Watching || m_phase == Phase::Checking;
}

CameraState CameraLightGate::Status() const {
    switch (m_phase) {
        case Phase::Idle:
            return CameraState::Off;
        case Phase::Resting:
            return CameraState::Dark;
        case Phase::Checking:
            return CameraState::Checking;
        case Phase::Watching:
            break;
    }
    if (m_seen.failed) {
        return CameraState::Unavailable;
    }
    return m_seen.running ? CameraState::On : CameraState::Starting;
}

void CameraLightGate::Watch(int64_t nowMs) {
    if (!Settled(nowMs) || m_seen.brightness >= kDarkLuma) {
        m_darkSinceMs = -1;
        return;
    }
    if (m_darkSinceMs < 0) {
        m_darkSinceMs = nowMs;
    }
    if (nowMs - m_darkSinceMs >= kDarkForMs) {
        Enter(Phase::Resting, nowMs);
    }
}

void CameraLightGate::Rest(int64_t nowMs) {
    if (nowMs - m_phaseSinceMs >= kRestMs) {
        Enter(Phase::Checking, nowMs);
    }
}

void CameraLightGate::Check(int64_t nowMs) {
    if (Settled(nowMs)) {
        Enter(m_seen.brightness >= kLightLuma ? Phase::Watching : Phase::Resting, nowMs);
    } else if (nowMs - m_phaseSinceMs >= kCheckGiveUpMs) {
        Enter(Phase::Resting, nowMs);
    }
}

bool CameraLightGate::Settled(int64_t nowMs) const {
    return m_seen.running && m_seen.brightness >= 0 && m_runningSinceMs >= 0 &&
           nowMs - m_runningSinceMs >= kSettleMs;
}

void CameraLightGate::Enter(Phase phase, int64_t nowMs) {
    m_phase = phase;
    m_phaseSinceMs = nowMs;
    m_darkSinceMs = -1;
}

}  // namespace gazeora
