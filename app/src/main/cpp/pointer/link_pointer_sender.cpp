// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "pointer/link_pointer_sender.h"

#include "pointer/queued_pointer_sender.h"

namespace gazeora {

std::unique_ptr<PointerSender> CreatePointerSender(std::shared_ptr<MessageLink> link) {
    return std::make_unique<QueuedPointerSender>(
        std::make_unique<LinkPointerSender>(std::move(link)));
}

}  // namespace gazeora
