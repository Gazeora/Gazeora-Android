#pragma once

#include <cstddef>
#include <cstdint>

#include "protocols/access_unit.h"

namespace gazeora {

// How to show a frame, as the PC sends it with each one in an SEI message (user data
// unregistered with the id "GazeoraViewInfo1", text after it; the PC's stream_view_info.h).
// A frame without it is shown as it is.
struct ViewInfo {
    // The phone does the lens correction for a VR viewer, with these lens settings: the
    // picture in each eye, barrelled as r' = r (1 + k1 r^2 + k2 r^4), `scale` of the eye's
    // height, lens centers `separation` of the screen width apart, red and blue `chroma`
    // larger and smaller.
    bool vr = false;
    // The frame holds two views side by side, the left eye's on the left, each squeezed to
    // half the width (3D).
    bool pair = false;
    float k1 = 0.34f;
    float k2 = 0.55f;
    float scale = 0.7f;
    float separation = 0.5f;
    float chroma = 0.0f;

    bool operator==(const ViewInfo& other) const {
        return vr == other.vr && pair == other.pair && k1 == other.k1 && k2 == other.k2 &&
               scale == other.scale && separation == other.separation && chroma == other.chroma;
    }
    bool operator!=(const ViewInfo& other) const { return !(*this == other); }
};

// The view info an access unit (Annex B) carries; a default one (shown as it is) if none.
ViewInfo FindViewInfo(VideoCodec codec, const uint8_t* data, size_t size);

}  // namespace gazeora
