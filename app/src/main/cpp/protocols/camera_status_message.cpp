// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "protocols/camera_status_message.h"

#include "protocols/gzf.h"

namespace gazeora {

std::vector<uint8_t> CameraStatusMessage::Write(const CameraReport& report) {
    std::vector<uint8_t> message(kGzfHeaderSize + kPayloadSize);
    GzfHeader header;
    header.type = uint8_t(GzfType::CameraStatus);
    header.tail = uint32_t(kPayloadSize);
    header.Write(message.data());
    message[kGzfHeaderSize] = uint8_t(report.state);
    message[kGzfHeaderSize + 1] = report.brightness;
    return message;
}

}  // namespace gazeora
