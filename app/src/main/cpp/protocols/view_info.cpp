// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "protocols/view_info.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "protocols/annex_b.h"

namespace gazeora {

namespace {

constexpr char kViewInfoId[] = "GazeoraViewInfo1";  // 16 bytes
constexpr uint8_t kH264Sei = 6;
constexpr uint8_t kH265PrefixSei = 39;
constexpr uint32_t kUserDataUnregistered = 5;

// The NAL unit's payload without its emulation prevention bytes (00 00 03 -> 00 00).
std::vector<uint8_t> Unescape(const uint8_t* data, size_t size) {
    std::vector<uint8_t> raw;
    raw.reserve(size);
    int zeros = 0;
    for (size_t i = 0; i < size; ++i) {
        if (zeros >= 2 && data[i] == 3) {
            zeros = 0;
            continue;
        }
        raw.push_back(data[i]);
        zeros = data[i] == 0 ? zeros + 1 : 0;
    }
    return raw;
}

// A number of the SEI syntax: 0xFF bytes adding 255 each, then the last byte.
bool ReadSeiNumber(const std::vector<uint8_t>& sei, size_t& at, uint32_t& value) {
    value = 0;
    while (at < sei.size() && sei[at] == 0xFF) {
        value += 255;
        ++at;
    }
    if (at >= sei.size()) {
        return false;
    }
    value += sei[at++];
    return true;
}

// "key=value key=value ..." into `info`; keys it doesn't know are skipped.
void ParseText(const std::string& text, ViewInfo& info) {
    size_t at = 0;
    while (at < text.size()) {
        size_t end = text.find(' ', at);
        if (end == std::string::npos) end = text.size();
        std::string pair = text.substr(at, end - at);
        at = end + 1;
        size_t equals = pair.find('=');
        if (equals == std::string::npos) {
            continue;
        }
        std::string key = pair.substr(0, equals);
        float value = std::strtof(pair.c_str() + equals + 1, nullptr);
        if (key == "src") info.app = pair.compare(equals + 1, std::string::npos, "app") == 0;
        else if (key == "vr") info.vr = value != 0;
        else if (key == "pair") info.pair = value != 0;
        else if (key == "k1") info.k1 = value;
        else if (key == "k2") info.k2 = value;
        else if (key == "scale") info.scale = value;
        else if (key == "sep") info.separation = value;
        else if (key == "chroma") info.chroma = value;
    }
}

}  // namespace

ViewInfo FindViewInfo(VideoCodec codec, const uint8_t* data, size_t size) {
    ViewInfo info;
    const size_t header = codec == VideoCodec::H265 ? 2 : 1;
    const uint8_t seiType = codec == VideoCodec::H265 ? kH265PrefixSei : kH264Sei;
    for (const NalSpan& span : SplitAnnexB(data, size)) {
        if (NalType(codec, data[span.nal]) != seiType || span.end - span.nal <= header) {
            continue;
        }
        std::vector<uint8_t> sei = Unescape(data + span.nal + header, span.end - span.nal - header);
        size_t at = 0;
        // Messages until the trailing bits.
        while (at < sei.size() && sei[at] != 0x80) {
            uint32_t type, length;
            // As `length > left`: `at + length` can wrap where size_t is 32 bits.
            if (!ReadSeiNumber(sei, at, type) || !ReadSeiNumber(sei, at, length) ||
                length > sei.size() - at) {
                break;
            }
            if (type == kUserDataUnregistered && length >= 16 &&
                std::memcmp(&sei[at], kViewInfoId, 16) == 0) {
                ParseText(std::string(sei.begin() + at + 16, sei.begin() + at + length), info);
                return info;
            }
            at += length;
        }
    }
    return info;
}

}  // namespace gazeora
