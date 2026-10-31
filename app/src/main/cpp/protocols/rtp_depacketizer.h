// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <vector>

#include "protocols/access_unit.h"

namespace gazeora {

// Puts video frames back together from RTP packets (RFC 6184 for H.264: single NAL units,
// STAP-A and FU-A; RFC 7798 for H.265: single, aggregation and fragmentation units). A frame
// ends at the packet with the marker bit, or when the timestamp moves on.
//
// Wi-Fi delivers some packets out of order, so one that comes early waits a few milliseconds
// for those before it. A packet that doesn't come is lost: its frame is dropped, and so is
// every frame after it up to the next key frame, as they refer to it and a decoder fed them
// shows garbage (green, on MediaTek phones). The source asks the PC for a key frame meanwhile.
class RtpDepacketizer {
public:
    using FrameSink = std::function<void(AccessUnit&&)>;
    using KeyFrameNeeded = std::function<void()>;

    RtpDepacketizer(VideoCodec codec, FrameSink onFrame, KeyFrameNeeded onKeyFrameNeeded)
        : m_codec(codec), m_onFrame(std::move(onFrame)),
          m_onKeyFrameNeeded(std::move(onKeyFrameNeeded)) {}

    // One RTP packet, header included, arriving at `nowMs` (any steady clock).
    void Add(const uint8_t* packet, size_t size, uint64_t nowMs = 0);
    // Gives up on packets that haven't come in time. Call often while Waiting.
    void Tick(uint64_t nowMs);
    // Whether packets that came early are waiting for one before them.
    bool Waiting() const { return !m_pending.empty(); }

    // For receiver reports (RFC 3550, 6.4.1).
    struct Reception {
        uint32_t ssrc = 0;              // the sender's
        uint32_t highestSequence = 0;   // extended with the wrap count
        uint32_t cumulativeLost = 0;
        uint8_t fractionLost = 0;       // since the last call, out of 256
    };
    bool Started() const { return m_started; }
    Reception TakeReception();

private:
    // Takes the packets waiting that are next in line.
    void Drain();
    // Goes on from the earliest packet waiting, the ones before it lost.
    void GiveUp(uint64_t nowMs);
    void Process(const uint8_t* packet, uint32_t timestamp, bool marker, size_t payload,
                 size_t end);
    void Payload(const uint8_t* payload, size_t size);
    void Append(const uint8_t* nal, size_t size);
    void Finish();

    VideoCodec m_codec;
    FrameSink m_onFrame;
    KeyFrameNeeded m_onKeyFrameNeeded;
    std::vector<uint8_t> m_frame;
    uint32_t m_timestamp = 0;
    bool m_broken = false;
    bool m_waitingForKeyFrame = false;
    bool m_inFragment = false;
    std::map<uint16_t, std::vector<uint8_t>> m_pending;  // by sequence number
    uint64_t m_gapSinceMs = 0;

    bool m_started = false;
    uint32_t m_ssrc = 0;
    uint16_t m_nextSequence = 0;
    uint32_t m_cycles = 0;
    uint16_t m_baseSequence = 0;
    uint16_t m_maxSequence = 0;
    uint32_t m_received = 0;
    uint32_t m_expectedAtReport = 0;
    uint32_t m_receivedAtReport = 0;
};

// An RTCP receiver report about `reception`, from `ourSsrc`.
std::vector<uint8_t> BuildReceiverReport(uint32_t ourSsrc,
                                         const RtpDepacketizer::Reception& reception);

// An RTCP Picture Loss Indication (RFC 4585, 6.3.1): `mediaSsrc` should send a key frame. It
// goes after a receiver report, in one compound packet.
std::vector<uint8_t> BuildPictureLossIndication(uint32_t ourSsrc, uint32_t mediaSsrc);

}  // namespace gazeora
