// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <mutex>
#include <string>

#include "links/message_link.h"
#include "net.h"

namespace gazeora {

// Input over the network: a datagram each to the PC's input port, whatever protocol the
// frames come over.
class UdpMessageLink final : public MessageLink {
public:
    explicit UdpMessageLink(std::string host) : m_host(std::move(host)) {}

    void Write(const std::vector<uint8_t>& message) override;

private:
    // Resolves the PC on first use, as a name lookup can take a while.
    bool Connect();

    std::string m_host;
    std::mutex m_mutex;
    net::Fd m_fd;  // with m_mutex
    bool m_failed = false;
};

}  // namespace gazeora
