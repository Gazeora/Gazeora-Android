// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <cstdint>
#include <memory>

#include "head/quaternion.h"
#include "links/message_link.h"

namespace gazeora {

// Head poses as HeadPose messages, on the link to the PC, numbered.
class HeadPoseSender {
public:
    // The numbers start somewhere new each time, so the PC can't take a new run's first
    // poses for late ones from the last run.
    explicit HeadPoseSender(std::shared_ptr<MessageLink> link);

    // From one thread at a time.
    void Send(const Quaternion& head);

private:
    std::shared_ptr<MessageLink> m_link;
    uint32_t m_sample;
};

}  // namespace gazeora
