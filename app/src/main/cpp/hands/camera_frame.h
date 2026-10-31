// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <cstdint>
#include <vector>

namespace gazeora {

// A YUV 4:2:0 picture as the camera gives it (AIMAGE_FORMAT_YUV_420_888): full-size luma,
// and chroma at half size each way, its samples `uvPixelStride` apart.
struct YuvPlanes {
    const uint8_t* y = nullptr;
    const uint8_t* u = nullptr;
    const uint8_t* v = nullptr;
    int32_t width = 0;
    int32_t height = 0;
    int32_t yRowStride = 0;
    int32_t uvRowStride = 0;
    int32_t uvPixelStride = 0;
};

// A square of the upright picture: around (x, y), `size` pixels on a side, turned `angle`
// radians clockwise as the picture shows it (x right, y down, in pixels from its top left).
struct PictureSquare {
    float x = 0;
    float y = 0;
    float size = 0;
    float angle = 0;
};

// RGBA rows, packed tightly.
struct RgbaFrame {
    std::vector<uint8_t> rgba;
    int32_t width = 0;
    int32_t height = 0;
};

namespace camera_frame {

// `yuv` turned to stand upright the way the room shows the camera: `toSensor` takes upright
// coordinates to the sensor's (CameraView::toSensor), so a quarter turn swaps the sides.
void UprightRgba(const YuvPlanes& yuv, const int toSensor[2][2], RgbaFrame& out);

// `square` of `yuv` stood upright as UprightRgba does, as `side` x `side` RGB floats from 0 to
// 1, rows down, brightened `gain` times (as far as white): what a hand tracking model looks at.
// Outside the picture it's black.
void CropRgb(const YuvPlanes& yuv, const int toSensor[2][2], const PictureSquare& square,
             int side, float gain, float* rgb);

// How bright `square` of the upright picture is on average, from 0 to 1, from a few of its
// pixels; 0 if it's all outside the picture.
float MeanLuma(const YuvPlanes& yuv, const int toSensor[2][2], const PictureSquare& square);

}  // namespace camera_frame

}  // namespace gazeora
