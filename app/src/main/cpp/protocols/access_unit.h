// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <cstdint>
#include <vector>

namespace gazeora {

enum class VideoCodec { H264, H265 };

// One encoded picture, as the decoder takes it: Annex B, a start code before each NAL unit.
struct AccessUnit {
    std::vector<uint8_t> data;
    VideoCodec codec = VideoCodec::H264;
    bool keyFrame = false;
    uint32_t timestamp = 0;  // 90 kHz
    // 0 when the protocol doesn't say (RTSP, HTTP): the decoder finds out from the stream.
    uint16_t width = 0;
    uint16_t height = 0;
};

}  // namespace gazeora
