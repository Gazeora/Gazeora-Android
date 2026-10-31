// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <cstdint>

#include "protocols/camera_status_message.h"

namespace gazeora {

// What the camera shows, now.
struct CameraSighting {
    bool running = false;
    bool failed = false;
    float brightness = -1;  // mean luma, 0..1; below 0 before the first picture
};

// State pattern: whether the back camera should be on, by the light it sees. In the dark it
// can't find hands anyway, so it goes off to save power, and comes on now and then for a
// moment to check whether the light came back. Different thresholds for dark and light, so a
// room in between doesn't switch it back and forth.
class CameraLightGate {
public:
    // Below this for kDarkForMs, the camera goes off.
    static constexpr float kDarkLuma = 0.10f;
    // A check must see at least this to keep it on.
    static constexpr float kLightLuma = 0.16f;
    static constexpr int64_t kDarkForMs = 3000;
    // How long it stays off between checks.
    static constexpr int64_t kRestMs = 10000;
    // Auto exposure needs this long after opening before the picture tells the light.
    static constexpr int64_t kSettleMs = 1000;
    // A check whose camera didn't open by then counts as dark.
    static constexpr int64_t kCheckGiveUpMs = 5000;

    // The hands are in use: the camera on.
    void Start(int64_t nowMs);
    // They aren't: off.
    void Stop();
    void Update(const CameraSighting& seen, int64_t nowMs);
    bool WantsCamera() const;
    CameraState Status() const;

private:
    enum class Phase { Idle, Watching, Resting, Checking };

    void Watch(int64_t nowMs);
    void Rest(int64_t nowMs);
    void Check(int64_t nowMs);
    // Whether the latest picture tells the light: running long enough for auto exposure.
    bool Settled(int64_t nowMs) const;
    void Enter(Phase phase, int64_t nowMs);

    Phase m_phase = Phase::Idle;
    int64_t m_phaseSinceMs = 0;
    int64_t m_runningSinceMs = -1;
    int64_t m_darkSinceMs = -1;
    CameraSighting m_seen;
};

}  // namespace gazeora
