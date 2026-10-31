// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <memory>

#include "links/message_link.h"
#include "pointer/pointer_sender.h"

namespace gazeora {

// Touches as pointer messages, on the link to the PC. A lost one is a missed move; the next
// one puts the pointer right.
class LinkPointerSender final : public PointerSender {
public:
    explicit LinkPointerSender(std::shared_ptr<MessageLink> link) : m_link(std::move(link)) {}

    void Send(const PointerEvent& event) override { m_link->Write(PointerMessage(event)); }

private:
    std::shared_ptr<MessageLink> m_link;
};

}  // namespace gazeora
