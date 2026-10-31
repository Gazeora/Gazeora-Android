// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <memory>

#include "links/message_link.h"
#include "protocols/camera_status_message.h"

namespace gazeora {

// The camera's status as CameraStatus messages, on the link to the PC.
class CameraStatusSender {
public:
    explicit CameraStatusSender(std::shared_ptr<MessageLink> link) : m_link(std::move(link)) {}

    void Send(const CameraReport& report);

private:
    std::shared_ptr<MessageLink> m_link;
};

}  // namespace gazeora
