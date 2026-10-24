#include "protocols/gzf.h"

#include <algorithm>

namespace gazeora {

namespace {

uint16_t Get16(const uint8_t* p) { return uint16_t(p[0] << 8 | p[1]); }

uint32_t Get32(const uint8_t* p) {
    return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3];
}

void Put16(uint8_t* p, uint16_t value) {
    p[0] = uint8_t(value >> 8);
    p[1] = uint8_t(value);
}

void Put32(uint8_t* p, uint32_t value) {
    for (int i = 0; i < 4; ++i) p[i] = uint8_t(value >> (24 - 8 * i));
}

}  // namespace

bool StartsWithGzfMagic(const uint8_t* data) {
    return data[0] == 'G' && data[1] == 'Z' && data[2] == 'F' && data[3] == '1';
}

bool GzfHeader::Read(const uint8_t* data, size_t size, GzfHeader& out) {
    if (size < kGzfHeaderSize || !StartsWithGzfMagic(data)) {
        return false;
    }
    out.type = data[4];
    out.flags = data[5];
    out.width = Get16(data + 6);
    out.height = Get16(data + 8);
    out.lossPerMille = Get16(data + 10);
    out.frameNumber = Get32(data + 12);
    out.timestamp = Get32(data + 16);
    out.tail = Get32(data + 20);
    return true;
}

void GzfHeader::Write(uint8_t* out) const {
    out[0] = 'G';
    out[1] = 'Z';
    out[2] = 'F';
    out[3] = '1';
    out[4] = type;
    out[5] = flags;
    Put16(out + 6, width);
    Put16(out + 8, height);
    Put16(out + 10, lossPerMille);
    Put32(out + 12, frameNumber);
    Put32(out + 16, timestamp);
    Put32(out + 20, tail);
}

std::array<uint8_t, kGzfHeaderSize> GzfMessage(GzfType type, int lossPerMille, uint32_t cookie) {
    GzfHeader header;
    header.type = uint8_t(type);
    header.timestamp = cookie;
    header.lossPerMille = uint16_t(std::clamp(lossPerMille, 0, 1000));
    std::array<uint8_t, kGzfHeaderSize> message;
    header.Write(message.data());
    return message;
}

}  // namespace gazeora
