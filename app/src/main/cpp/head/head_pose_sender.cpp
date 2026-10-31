// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "head/head_pose_sender.h"

#include <random>

#include "protocols/head_pose_message.h"

namespace gazeora {

HeadPoseSender::HeadPoseSender(std::shared_ptr<MessageLink> link)
    : m_link(std::move(link)), m_sample(std::random_device()()) {}

void HeadPoseSender::Send(const Quaternion& head) {
    HeadPose pose;
    pose.sample = ++m_sample;
    pose.x = float(head.x);
    pose.y = float(head.y);
    pose.z = float(head.z);
    pose.w = float(head.w);
    m_link->Write(HeadPoseMessage(pose));
}

}  // namespace gazeora
