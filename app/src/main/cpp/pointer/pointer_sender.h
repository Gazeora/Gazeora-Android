// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <memory>

#include "links/message_link.h"
#include "protocols/pointer_message.h"

namespace gazeora {

// Where touches go to reach the PC's mouse.
class PointerSender {
public:
    virtual ~PointerSender() = default;
    virtual void Send(const PointerEvent& event) = 0;
};

// Factory: touches on `link`, queued so Send never blocks its caller.
std::unique_ptr<PointerSender> CreatePointerSender(std::shared_ptr<MessageLink> link);

}  // namespace gazeora
