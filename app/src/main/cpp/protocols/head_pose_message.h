// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace gazeora {

// Where the viewer's head points: a unit quaternion turning the head's axes into the world's.
// The world has Y up and a heading of its own; the head looks down its -Z, with X to its right
// and Y up, as OpenGL's camera.
struct HeadPose {
    // Counts up with every pose sent, so the PC can tell a late one from a new one.
    uint32_t sample = 0;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;
};

// A GZF1 HeadPose message (gzf.h): the header, its last word the payload length, then a
// 12-byte big-endian payload: the sample as u32, then x, y, z and w as i16, -1..1 as
// -32767..32767. Sent where pointer messages go. The PC has its own copy (Gazeora:
// head_pose_message.h); keep the two in step.
constexpr size_t kHeadPosePayloadSize = 12;

std::vector<uint8_t> HeadPoseMessage(const HeadPose& pose);

}  // namespace gazeora
