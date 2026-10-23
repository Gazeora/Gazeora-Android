#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "protocols/access_unit.h"

namespace gazeora {

// A NAL unit in an Annex B buffer: where its start code begins, where the unit itself (its
// header) begins, and where it ends.
struct NalSpan {
    size_t start;
    size_t nal;
    size_t end;
};

std::vector<NalSpan> SplitAnnexB(const uint8_t* data, size_t size);

uint8_t NalType(VideoCodec codec, uint8_t firstByte);
// IDR for H.264; IDR, CRA and BLA for H.265: a picture decodable on its own.
bool IsKeyNal(VideoCodec codec, uint8_t type);
bool ContainsKeyFrame(VideoCodec codec, const uint8_t* data, size_t size);

// The parameter sets of a key frame, start codes included, as MediaCodec wants them: SPS in
// csd-0 and PPS in csd-1 for H.264, VPS, SPS and PPS together in csd-0 for H.265. Empty if
// the frame doesn't carry them.
struct ParameterSets {
    std::vector<uint8_t> csd0;
    std::vector<uint8_t> csd1;

    bool Empty() const { return csd0.empty(); }
    bool operator==(const ParameterSets& other) const {
        return csd0 == other.csd0 && csd1 == other.csd1;
    }
    bool operator!=(const ParameterSets& other) const { return !(*this == other); }
};
ParameterSets ExtractParameterSets(VideoCodec codec, const uint8_t* data, size_t size);

}  // namespace gazeora
