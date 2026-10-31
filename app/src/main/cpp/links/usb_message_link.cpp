// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "links/usb_message_link.h"

#include "sources/usb_link.h"

namespace gazeora {

void UsbMessageLink::Write(const std::vector<uint8_t>& message) { UsbLink::Get().Write(message); }

}  // namespace gazeora
