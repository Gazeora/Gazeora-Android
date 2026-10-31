// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "protocols/head_pose_message.h"

#include <algorithm>
#include <cmath>

#include "protocols/gzf.h"

namespace gazeora {

namespace {

constexpr float kComponentRange = 32767.0f;
constexpr size_t kSampleAt = 0;
constexpr size_t kQuaternionAt = 4;
constexpr size_t kComponentSize = 2;

void Put16(uint8_t* p, uint16_t value) {
    p[0] = uint8_t(value >> 8);
    p[1] = uint8_t(value);
}

void Put32(uint8_t* p, uint32_t value) {
    p[0] = uint8_t(value >> 24);
    p[1] = uint8_t(value >> 16);
    p[2] = uint8_t(value >> 8);
    p[3] = uint8_t(value);
}

uint16_t Component(float value) {
    return uint16_t(int16_t(std::lround(std::clamp(value, -1.0f, 1.0f) * kComponentRange)));
}

}  // namespace

std::vector<uint8_t> HeadPoseMessage(const HeadPose& pose) {
    std::vector<uint8_t> message(kGzfHeaderSize + kHeadPosePayloadSize);
    GzfHeader header;
    header.type = uint8_t(GzfType::HeadPose);
    header.tail = uint32_t(kHeadPosePayloadSize);
    header.Write(message.data());
    uint8_t* payload = message.data() + kGzfHeaderSize;
    uint8_t* q = payload + kQuaternionAt;
    Put32(payload + kSampleAt, pose.sample);
    Put16(q, Component(pose.x));
    Put16(q + kComponentSize, Component(pose.y));
    Put16(q + 2 * kComponentSize, Component(pose.z));
    Put16(q + 3 * kComponentSize, Component(pose.w));
    return message;
}

}  // namespace gazeora
