// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "hands/light_estimate.h"

#include <algorithm>
#include <cstdint>

namespace gazeora {

namespace light_estimate {

namespace {

// Every this many pixels each way: the light doesn't need them all.
constexpr int32_t kStep = 4;
// The brightest share of the picture is where a light in view would be; it's a light only if
// it's this many times the picture's mean, and wholly the light from this many times.
constexpr float kBrightShare = 0.005f;
constexpr float kSpotContrast = 1.8f;
constexpr float kSpotFull = 3.5f;
// A light out of view comes from the picture's brighter side: this much toward it for each
// share brighter, a little from above as most rooms' lights are, and from in front of the
// hands, where the viewer is.
constexpr float kSideGain = 2.0f;
constexpr float kFromAbove = -0.3f;
constexpr float kFromFront = -0.6f;
// How much of the picture's color the light keeps; the rest is white.
constexpr float kTintKept = 0.5f;
// The main light's strength and the light all round, for a picture from black to white: the
// hands stay in sight in a dark room.
constexpr float kLeastStrength = 0.6f;
constexpr float kMostStrength = 1.3f;
constexpr float kLeastAmbient = 0.22f;
constexpr float kMostAmbient = 0.5f;

float Luma(const uint8_t* p) { return 0.299f * p[0] + 0.587f * p[1] + 0.114f * p[2]; }

}  // namespace

SceneLight Estimate(const RgbaFrame& frame, float tanAcross, float tanDown) {
    SceneLight light;
    const int32_t width = frame.width, height = frame.height;
    if (width < kStep || height < kStep ||
        frame.rgba.size() < size_t(width) * size_t(height) * 4) {
        return light;
    }
    double sum[3] = {0, 0, 0}, luma = 0, left = 0, right = 0, top = 0, bottom = 0;
    int histogram[256] = {};
    int samples = 0, leftSamples = 0, topSamples = 0;
    for (int32_t y = 0; y < height; y += kStep) {
        for (int32_t x = 0; x < width; x += kStep) {
            const uint8_t* p = &frame.rgba[(size_t(y) * width + x) * 4];
            const float l = Luma(p);
            for (int c = 0; c < 3; ++c) sum[c] += p[c];
            luma += l;
            ++histogram[std::min(int(l), 255)];
            ++samples;
            if (x < width / 2) {
                left += l;
                ++leftSamples;
            } else {
                right += l;
            }
            if (y < height / 2) {
                top += l;
                ++topSamples;
            } else {
                bottom += l;
            }
        }
    }
    const double mean = luma / samples;
    if (mean < 1 || leftSamples == samples || topSamples == samples) {
        return light;  // black: nothing to go by
    }
    // Where the brightest part is, and how much brighter.
    int threshold = 255;
    for (int counted = 0; threshold > 0; --threshold) {
        counted += histogram[threshold];
        if (counted >= kBrightShare * samples) break;
    }
    double spotX = 0, spotY = 0, spotWeight = 0;
    for (int32_t y = 0; y < height; y += kStep) {
        for (int32_t x = 0; x < width; x += kStep) {
            const float l = Luma(&frame.rgba[(size_t(y) * width + x) * 4]);
            if (l >= threshold) {
                const double weight = l - threshold + 1;
                spotX += weight * (x + 0.5) / width;
                spotY += weight * (y + 0.5) / height;
                spotWeight += weight;
            }
        }
    }
    const float contrast = float(threshold / mean);
    const float spotShare =
        std::clamp((contrast - kSpotContrast) / (kSpotFull - kSpotContrast), 0.0f, 1.0f);
    const Vec3 spot = spotWeight > 0 ? Normalize({float(2 * spotX / spotWeight - 1) * tanAcross,
                                                  float(2 * spotY / spotWeight - 1) * tanDown, 1},
                                                 light.from)
                                     : light.from;
    const float across = float((right / (samples - leftSamples) - left / leftSamples) / mean);
    const float down = float((bottom / (samples - topSamples) - top / topSamples) / mean);
    const Vec3 side = Normalize({across * kSideGain, down * kSideGain + kFromAbove, kFromFront},
                                light.from);
    light.from = Normalize(side * (1 - spotShare) + spot * spotShare, light.from);

    const double brightest = std::max({sum[0], sum[1], sum[2]});
    for (int c = 0; c < 3; ++c) {
        light.color[c] = float(1 - kTintKept + kTintKept * sum[c] / brightest);
    }
    const float level = float(mean / 255);
    light.strength = kLeastStrength + (kMostStrength - kLeastStrength) * level;
    light.ambient = kLeastAmbient + (kMostAmbient - kLeastAmbient) * level;
    return light;
}

SceneLight Blend(const SceneLight& from, const SceneLight& to, float share) {
    SceneLight light;
    light.from = Normalize(from.from + (to.from - from.from) * share, to.from);
    for (int c = 0; c < 3; ++c) {
        light.color[c] = from.color[c] + (to.color[c] - from.color[c]) * share;
    }
    light.strength = from.strength + (to.strength - from.strength) * share;
    light.ambient = from.ambient + (to.ambient - from.ambient) * share;
    return light;
}

}  // namespace light_estimate

}  // namespace gazeora
