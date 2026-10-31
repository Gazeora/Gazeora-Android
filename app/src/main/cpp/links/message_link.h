// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace gazeora {

// The way the player's input messages (touches, head poses) reach the PC. From any thread.
class MessageLink {
public:
    virtual ~MessageLink() = default;
    // One whole GZF1 message. A lost one isn't sent again.
    virtual void Write(const std::vector<uint8_t>& message) = 0;
};

// Factory: the link for a stream `protocol` (as Player::Start takes it).
std::shared_ptr<MessageLink> CreateMessageLink(const std::string& protocol,
                                               const std::string& host);

}  // namespace gazeora
