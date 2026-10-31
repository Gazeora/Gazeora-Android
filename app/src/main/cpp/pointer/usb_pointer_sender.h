// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include "pointer/pointer_sender.h"

namespace gazeora {

// Touches on the USB cable, alongside the player's hellos (sources/usb_link.h).
class UsbPointerSender final : public PointerSender {
public:
    void Send(const PointerEvent& event) override;
};

}  // namespace gazeora
