// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "protocols/pointer_message.h"

#include <algorithm>
#include <cmath>

#include "protocols/gzf.h"

namespace gazeora {

namespace {

constexpr float kCoordinateRange = 65535.0f;

void Put16(uint8_t* p, uint16_t value) {
    p[0] = uint8_t(value >> 8);
    p[1] = uint8_t(value);
}

uint16_t Coordinate(float fraction) {
    return uint16_t(std::lround(std::clamp(fraction, 0.0f, 1.0f) * kCoordinateRange));
}

uint16_t Wheel(int units) { return uint16_t(int16_t(std::clamp(units, -32768, 32767))); }

}  // namespace

std::vector<uint8_t> PointerMessage(const PointerEvent& event) {
    std::vector<uint8_t> message(kGzfHeaderSize + kPointerPayloadSize);
    GzfHeader header;
    header.type = uint8_t(GzfType::Pointer);
    header.tail = uint32_t(kPointerPayloadSize);
    header.Write(message.data());
    uint8_t* payload = message.data() + kGzfHeaderSize;
    payload[0] = uint8_t(event.action);
    payload[1] = uint8_t(event.button);
    Put16(payload + 2, Coordinate(event.x));
    Put16(payload + 4, Coordinate(event.y));
    Put16(payload + 6, Wheel(event.wheelVertical));
    Put16(payload + 8, Wheel(event.wheelHorizontal));
    return message;
}

}  // namespace gazeora
