// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace gazeora {

// How the back camera stands. The numbers are on the wire.
enum class CameraState : uint8_t {
    Off = 0,          // the hands aren't in use
    Starting = 1,     // opening
    On = 2,           // looking for hands
    Dark = 3,         // off to save power: too dark to see hands
    Checking = 4,     // on for a moment, to see whether the light came back
    Unavailable = 5,  // couldn't open: no permission, in use elsewhere, or none
};

struct CameraReport {
    CameraState state = CameraState::Off;
    uint8_t brightness = 0;  // the picture's mean luma, 0..255, as last seen
};

// A GZF1 CameraStatus message (gzf.h): the header, its last word the payload length, then a
// 2-byte payload: state, brightness. Sent where pointer messages go. The PC has its own copy
// (Gazeora: camera_status_message.h); keep the two in step.
class CameraStatusMessage {
public:
    static constexpr size_t kPayloadSize = 2;

    static std::vector<uint8_t> Write(const CameraReport& report);
};

}  // namespace gazeora
