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
// player's loss per mille (in a hello), frame number, 90 kHz timestamp, and a last word that
// is the chunk index and count over UDP, the payload length over USB.
constexpr size_t kGzfHeaderSize = 24;
constexpr size_t kGzfMaxDatagram = 1400;
// How often the player says hello; the PC drops a player silent for 5 s.
constexpr int kGzfHelloEveryMs = 1000;

enum class GzfType : uint8_t { Frame = 1, Hello = 2, KeyFrameRequest = 3, Bye = 4 };

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

// A message from the player: a hello, key frame request or bye, without payload.
std::array<uint8_t, kGzfHeaderSize> GzfMessage(GzfType type, int lossPerMille);

}  // namespace gazeora
