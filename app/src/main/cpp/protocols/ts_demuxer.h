#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

#include "protocols/access_unit.h"

namespace gazeora {

// Takes the video out of an MPEG transport stream (Gazeora's HTTP protocol: ts_muxer.h on the
// PC): the PAT names the PMT, the PMT the first H.264 or H.265 stream, and each of its PES
// packets is one frame.
//
// A frame goes out as soon as it's whole rather than when the next one starts, a frame
// earlier: the packet that ends a PES carries adaptation field stuffing, as a muxer pads the
// last one. A PES that happens to fill its last packet exactly goes out with the next one.
class TsDemuxer {
public:
    using FrameSink = std::function<void(AccessUnit&&)>;

    explicit TsDemuxer(FrameSink onFrame) : m_onFrame(std::move(onFrame)) {}

    // Any number of bytes; a partial packet waits for the rest.
    void Add(const uint8_t* data, size_t size);

private:
    void Packet(const uint8_t* packet);
    void Pat(const uint8_t* section, size_t size);
    void Pmt(const uint8_t* section, size_t size);
    void Flush();

    FrameSink m_onFrame;
    std::vector<uint8_t> m_pending;  // a partial packet
    int m_pmtPid = -1;
    int m_videoPid = -1;
    VideoCodec m_codec = VideoCodec::H264;
    std::vector<uint8_t> m_pes;
    bool m_randomAccess = false;
    bool m_broken = false;
    int m_continuity = -1;
};

}  // namespace gazeora
