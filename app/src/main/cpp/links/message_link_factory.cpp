// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "links/message_link.h"

#include "links/udp_message_link.h"
#include "links/usb_message_link.h"

namespace gazeora {

std::shared_ptr<MessageLink> CreateMessageLink(const std::string& protocol,
                                               const std::string& host) {
    if (protocol == "usb") {
        return std::make_shared<UsbMessageLink>();
    }
    return std::make_shared<UdpMessageLink>(host);
}

}  // namespace gazeora
