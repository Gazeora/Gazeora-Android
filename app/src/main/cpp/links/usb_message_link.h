// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include "links/message_link.h"

namespace gazeora {

// Input on the USB cable, alongside the player's hellos (sources/usb_link.h).
class UsbMessageLink final : public MessageLink {
public:
    void Write(const std::vector<uint8_t>& message) override;
};

}  // namespace gazeora
