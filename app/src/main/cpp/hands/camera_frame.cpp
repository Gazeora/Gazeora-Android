// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "hands/camera_frame.h"

#include <algorithm>
#include <cmath>

namespace gazeora {

namespace {

constexpr int kOpaque = 255;
constexpr int kChromaMiddle = 128;

uint8_t Channel(int value) { return uint8_t(std::clamp(value, 0, 255)); }

// How many pixels each way MeanLuma looks at.
constexpr int kLumaSamples = 16;

// Where a square's pixels are in the sensor's picture: pixel (column, row) of the square cut
// into `side` x `side` is at `at` + column * `perColumn` + row * `perRow`, in the sensor's
// pixels from its top left corner (not from its first pixel's middle).
struct SensorWalk {
    float at[2];
    float perColumn[2];
    float perRow[2];
};

SensorWalk Walk(const YuvPlanes& yuv, const int toSensor[2][2], const PictureSquare& square,
                int side) {
    const bool quarter = toSensor[0][0] == 0;
    const float width = float(quarter ? yuv.height : yuv.width);
    const float height = float(quarter ? yuv.width : yuv.height);
    const float step = square.size / side;
    const float c = std::cos(square.angle), s = std::sin(square.angle);
    // The first pixel's middle, from the square's, and the steps to the next, upright.
    const float first = step / 2 - square.size / 2;
    const float upright[2] = {square.x + (c - s) * first - width / 2,
                              square.y + (s + c) * first - height / 2};
    const float column[2] = {c * step, s * step};
    const float row[2] = {-s * step, c * step};
    SensorWalk walk;
    const float middle[2] = {yuv.width / 2.0f, yuv.height / 2.0f};
    for (int k = 0; k < 2; ++k) {
        walk.at[k] = toSensor[k][0] * upright[0] + toSensor[k][1] * upright[1] + middle[k];
        walk.perColumn[k] = toSensor[k][0] * column[0] + toSensor[k][1] * column[1];
        walk.perRow[k] = toSensor[k][0] * row[0] + toSensor[k][1] * row[1];
    }
    return walk;
}

}  // namespace

namespace camera_frame {

void UprightRgba(const YuvPlanes& yuv, const int toSensor[2][2], RgbaFrame& out) {
    const bool quarter = toSensor[0][0] == 0;
    out.width = quarter ? yuv.height : yuv.width;
    out.height = quarter ? yuv.width : yuv.height;
    out.rgba.resize(size_t(out.width) * out.height * 4);
    uint8_t* pixel = out.rgba.data();
    for (int32_t row = 0; row < out.height; ++row) {
        for (int32_t column = 0; column < out.width; ++column) {
            // Twice the offset from the middle, so pixel centers stay whole numbers as they
            // turn.
            int x = 2 * column + 1 - out.width, y = 2 * row + 1 - out.height;
            int sensorX = (toSensor[0][0] * x + toSensor[0][1] * y + yuv.width - 1) / 2;
            int sensorY = (toSensor[1][0] * x + toSensor[1][1] * y + yuv.height - 1) / 2;
            int luma = yuv.y[sensorY * yuv.yRowStride + sensorX];
            int at = (sensorY / 2) * yuv.uvRowStride + (sensorX / 2) * yuv.uvPixelStride;
            int u = yuv.u[at] - kChromaMiddle, v = yuv.v[at] - kChromaMiddle;
            // BT.601, full range (JFIF), as cameras give it; in 256ths.
            pixel[0] = Channel(luma + (359 * v) / 256);
            pixel[1] = Channel(luma - (88 * u + 183 * v) / 256);
            pixel[2] = Channel(luma + (454 * u) / 256);
            pixel[3] = kOpaque;
            pixel += 4;
        }
    }
}

void CropRgb(const YuvPlanes& yuv, const int toSensor[2][2], const PictureSquare& square,
             int side, float gain, float* rgb) {
    const SensorWalk walk = Walk(yuv, toSensor, square, side);
    const float scale = gain / 255;
    for (int row = 0; row < side; ++row) {
        float x = walk.at[0] + row * walk.perRow[0];
        float y = walk.at[1] + row * walk.perRow[1];
        for (int column = 0; column < side; ++column, x += walk.perColumn[0],
                 y += walk.perColumn[1], rgb += 3) {
            if (x < 0 || y < 0 || x >= yuv.width || y >= yuv.height) {
                rgb[0] = rgb[1] = rgb[2] = 0;
                continue;
            }
            // Luma between the four nearest pixels' middles; chroma from the nearest.
            const float px = std::max(x - 0.5f, 0.0f), py = std::max(y - 0.5f, 0.0f);
            const int x0 = std::min(int(px), yuv.width - 2), y0 = std::min(int(py), yuv.height - 2);
            const float fx = std::min(px - x0, 1.0f), fy = std::min(py - y0, 1.0f);
            const uint8_t* top = yuv.y + y0 * yuv.yRowStride + x0;
            const uint8_t* bottom = top + yuv.yRowStride;
            const float luma = (top[0] + (top[1] - top[0]) * fx) * (1 - fy) +
                               (bottom[0] + (bottom[1] - bottom[0]) * fx) * fy;
            const int at = (int(y) / 2) * yuv.uvRowStride + (int(x) / 2) * yuv.uvPixelStride;
            const float u = float(yuv.u[at] - kChromaMiddle), v = float(yuv.v[at] - kChromaMiddle);
            // BT.601, full range, as in UprightRgba.
            rgb[0] = std::min((luma + 1.402f * v) * scale, 1.0f);
            rgb[1] = std::min((luma - 0.344f * u - 0.714f * v) * scale, 1.0f);
            rgb[2] = std::min((luma + 1.772f * u) * scale, 1.0f);
            for (int k = 0; k < 3; ++k) rgb[k] = std::max(rgb[k], 0.0f);
        }
    }
}

float MeanLuma(const YuvPlanes& yuv, const int toSensor[2][2], const PictureSquare& square) {
    const SensorWalk walk = Walk(yuv, toSensor, square, kLumaSamples);
    int sum = 0, count = 0;
    for (int row = 0; row < kLumaSamples; ++row) {
        for (int column = 0; column < kLumaSamples; ++column) {
            const float x = walk.at[0] + column * walk.perColumn[0] + row * walk.perRow[0];
            const float y = walk.at[1] + column * walk.perColumn[1] + row * walk.perRow[1];
            if (x < 0 || y < 0 || x >= yuv.width || y >= yuv.height) continue;
            sum += yuv.y[int(y) * yuv.yRowStride + int(x)];
            ++count;
        }
    }
    return count > 0 ? float(sum) / count / 255 : 0.0f;
}

}  // namespace camera_frame

}  // namespace gazeora
