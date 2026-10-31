// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace gazeora {

// The "GZF1" wire format of the UDP and USB protocols: encoded frames with a small header, no
// RTSP or RTP. The PC has its own copy (Gazeora: frame_packet.h, which describes it in full);
// keep the two in step.
//
// A 24-byte big-endian header: "GZF1", type, flags (key frame, H.265), width, height, the
// player's loss per mille (in a hello), frame number, 90 kHz timestamp (in the player's
// messages over UDP and the PC's challenge: the cookie), and a last word that is the chunk index
// and count over UDP, the payload length over USB.
//
// Over UDP the PC answers a hello with a challenge, whose cookie the player's messages then
// carry: the PC streams only to an address that shows it got it, so one can't name another.
constexpr size_t kGzfHeaderSize = 24;
constexpr size_t kGzfMaxDatagram = 1400;
// How often the player says hello; the PC drops a player silent for 5 s.
constexpr int kGzfHelloEveryMs = 1000;

// Pointer: a touch for the PC's mouse (pointer_message.h). Challenge: the PC's cookie (UDP).
enum class GzfType : uint8_t {
    Frame = 1,
    Hello = 2,
    KeyFrameRequest = 3,
    Bye = 4,
    Pointer = 5,
    Challenge = 6,
};

constexpr uint8_t kGzfKeyFrame = 1;
constexpr uint8_t kGzfH265 = 2;

struct GzfHeader {
    uint8_t type = 0;
    uint8_t flags = 0;
    uint16_t width = 0;
    uint16_t height = 0;
    uint16_t lossPerMille = 0;
    uint32_t frameNumber = 0;
    uint32_t timestamp = 0;
    uint32_t tail = 0;

    bool KeyFrame() const { return flags & kGzfKeyFrame; }
    bool H265() const { return flags & kGzfH265; }

    // False if there isn't a whole header or it isn't GZF1.
    static bool Read(const uint8_t* data, size_t size, GzfHeader& out);
    void Write(uint8_t* out) const;
};

bool StartsWithGzfMagic(const uint8_t* data);

// A message from the player: a hello, key frame request or bye, without payload; over UDP with
// the cookie of the PC's last challenge (0 before one).
std::array<uint8_t, kGzfHeaderSize> GzfMessage(GzfType type, int lossPerMille, uint32_t cookie = 0);

}  // namespace gazeora
