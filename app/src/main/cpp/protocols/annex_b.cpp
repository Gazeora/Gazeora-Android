#include "protocols/annex_b.h"

namespace gazeora {

namespace {

// Where the next 00 00 01 is at or after `from`, or `size`.
size_t FindStartCode(const uint8_t* data, size_t size, size_t from) {
    for (size_t i = from; i + 3 <= size; ++i) {
        if (data[i + 2] > 1) {
            i += 2;  // none of these three bytes can begin one
        } else if (data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 1) {
            return i;
        }
    }
    return size;
}

}  // namespace

std::vector<NalSpan> SplitAnnexB(const uint8_t* data, size_t size) {
    std::vector<NalSpan> nals;
    size_t code = FindStartCode(data, size, 0);
    while (code < size) {
        size_t nal = code + 3;
        size_t next = FindStartCode(data, size, nal);
        size_t end = next;
        if (next < size && end > nal && data[end - 1] == 0) {
            --end;  // the next one is a 4-byte start code
        }
        size_t start = code > 0 && data[code - 1] == 0 ? code - 1 : code;
        if (!nals.empty() && start < nals.back().end) {
            start = nals.back().end;
        }
        if (nal < end) {
            nals.push_back({start, nal, end});
        }
        code = next;
    }
    return nals;
}

uint8_t NalType(VideoCodec codec, uint8_t firstByte) {
    return codec == VideoCodec::H265 ? uint8_t((firstByte >> 1) & 0x3F) : uint8_t(firstByte & 0x1F);
}

bool IsKeyNal(VideoCodec codec, uint8_t type) {
    return codec == VideoCodec::H265 ? type >= 16 && type <= 21 : type == 5;
}

bool ContainsKeyFrame(VideoCodec codec, const uint8_t* data, size_t size) {
    for (const NalSpan& span : SplitAnnexB(data, size)) {
        if (IsKeyNal(codec, NalType(codec, data[span.nal]))) {
            return true;
        }
    }
    return false;
}

ParameterSets ExtractParameterSets(VideoCodec codec, const uint8_t* data, size_t size) {
    ParameterSets sets;
    std::vector<uint8_t> pps;
    for (const NalSpan& span : SplitAnnexB(data, size)) {
        uint8_t type = NalType(codec, data[span.nal]);
        std::vector<uint8_t>* into = nullptr;
        if (codec == VideoCodec::H265) {
            if (type >= 32 && type <= 34) {  // VPS, SPS, PPS
                into = &sets.csd0;
            }
        } else if (type == 7) {
            into = &sets.csd0;
        } else if (type == 8) {
            into = &pps;
        }
        if (into) {
            static const uint8_t kStartCode[] = {0, 0, 0, 1};
            into->insert(into->end(), kStartCode, kStartCode + 4);
            into->insert(into->end(), data + span.nal, data + span.end);
        }
    }
    if (codec == VideoCodec::H264) {
        if (pps.empty()) {
            sets.csd0.clear();  // no use without both
        } else if (!sets.csd0.empty()) {
            sets.csd1 = std::move(pps);
        }
    }
    return sets;
}

}  // namespace gazeora
