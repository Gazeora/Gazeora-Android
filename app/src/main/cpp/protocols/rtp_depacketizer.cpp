// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "protocols/rtp_depacketizer.h"

#include "protocols/annex_b.h"

namespace gazeora {

namespace {

constexpr size_t kRtpHeaderSize = 12;
constexpr uint8_t kStartCode[] = {0, 0, 0, 1};

// H.264 packet types.
constexpr uint8_t kStapA = 24;
constexpr uint8_t kFuA = 28;
// H.265 packet types.
constexpr uint8_t kAggregation = 48;
constexpr uint8_t kFragmentation = 49;

// Packets that came early wait this long, or until there are this many, for the ones before
// them: Wi-Fi delivers some out of order.
constexpr uint64_t kReorderWaitMs = 10;
constexpr size_t kMaxReorderPackets = 64;
// A frame no encoder makes: one never ended (no marker, the same timestamp) is dropped.
constexpr size_t kMaxFrameBytes = 16 << 20;

struct RtpHeader {
    uint16_t sequence;
    uint32_t timestamp;
    uint32_t ssrc;
    bool marker;
    size_t payload;  // where the payload starts and ends
    size_t end;
};

bool ParseRtp(const uint8_t* packet, size_t size, RtpHeader& out) {
    if (size < kRtpHeaderSize || (packet[0] >> 6) != 2) {
        return false;
    }
    bool padding = packet[0] & 0x20;
    bool extension = packet[0] & 0x10;
    size_t at = kRtpHeaderSize + 4 * size_t(packet[0] & 0x0F);
    out.marker = packet[1] & 0x80;
    out.sequence = uint16_t(packet[2] << 8 | packet[3]);
    out.timestamp =
        uint32_t(packet[4]) << 24 | uint32_t(packet[5]) << 16 | uint32_t(packet[6]) << 8 | packet[7];
    out.ssrc =
        uint32_t(packet[8]) << 24 | uint32_t(packet[9]) << 16 | uint32_t(packet[10]) << 8 | packet[11];
    if (extension) {
        if (at + 4 > size) {
            return false;
        }
        at += 4 + 4 * size_t(packet[at + 2] << 8 | packet[at + 3]);
    }
    size_t end = size;
    if (padding && end > 0) {
        size_t pad = packet[end - 1];
        end = pad <= end ? end - pad : 0;
    }
    out.payload = at;
    out.end = end;
    return at < end;
}

}  // namespace

void RtpDepacketizer::Add(const uint8_t* packet, size_t size, uint64_t nowMs) {
    RtpHeader header;
    if (!ParseRtp(packet, size, header)) {
        return;
    }
    if (!m_started || header.ssrc != m_ssrc) {
        m_started = true;
        m_ssrc = header.ssrc;
        m_baseSequence = m_maxSequence = m_nextSequence = header.sequence;
        m_cycles = 0;
        m_received = m_expectedAtReport = m_receivedAtReport = 0;
        m_pending.clear();
        // The PC starts a player on a whole frame; one joined mid-fragment is caught below.
        m_frame.clear();
        m_broken = false;
        m_inFragment = false;
        m_timestamp = header.timestamp;
    }
    int16_t ahead = int16_t(header.sequence - m_nextSequence);
    if (ahead < 0) {
        return;  // too late, given up on; or repeated
    }
    ++m_received;
    if (int16_t(header.sequence - m_maxSequence) > 0) {
        if (header.sequence < m_maxSequence) {
            m_cycles += 1u << 16;
        }
        m_maxSequence = header.sequence;
    }
    if (ahead == 0) {
        Process(packet, header.timestamp, header.marker, header.payload, header.end);
        ++m_nextSequence;
        Drain();
        return;
    }
    // Early: it waits for the ones before it.
    if (m_pending.empty()) {
        m_gapSinceMs = nowMs;
    }
    m_pending.emplace(header.sequence, std::vector<uint8_t>(packet, packet + size));
    if (m_pending.size() > kMaxReorderPackets) {
        GiveUp(nowMs);
    }
}

void RtpDepacketizer::Tick(uint64_t nowMs) {
    if (!m_pending.empty() && nowMs - m_gapSinceMs >= kReorderWaitMs) {
        GiveUp(nowMs);
    }
}

void RtpDepacketizer::Drain() {
    for (auto it = m_pending.find(m_nextSequence); it != m_pending.end();
         it = m_pending.find(m_nextSequence)) {
        RtpHeader header;
        std::vector<uint8_t> packet = std::move(it->second);
        m_pending.erase(it);
        ParseRtp(packet.data(), packet.size(), header);
        Process(packet.data(), header.timestamp, header.marker, header.payload, header.end);
        ++m_nextSequence;
    }
}

void RtpDepacketizer::GiveUp(uint64_t nowMs) {
    // The packets before the earliest one waiting aren't coming: on from it.
    uint16_t earliest = m_pending.begin()->first;
    for (const auto& entry : m_pending) {
        if (int16_t(entry.first - m_nextSequence) < int16_t(earliest - m_nextSequence)) {
            earliest = entry.first;
        }
    }
    m_nextSequence = earliest;
    m_broken = true;
    Drain();
    m_gapSinceMs = nowMs;  // for a gap further on
}

void RtpDepacketizer::Process(const uint8_t* packet, uint32_t timestamp, bool marker,
                              size_t payload, size_t end) {
    if (timestamp != m_timestamp) {
        Finish();  // the last frame's marker was lost: it's broken, so this drops it
        m_timestamp = timestamp;
    }
    Payload(packet + payload, end - payload);
    if (m_frame.size() > kMaxFrameBytes) {
        m_frame.clear();
        m_broken = true;
    }
    if (marker) {
        Finish();
    }
}

void RtpDepacketizer::Payload(const uint8_t* p, size_t size) {
    if (m_codec == VideoCodec::H265) {
        if (size < 3) {
            return;
        }
        uint8_t type = (p[0] >> 1) & 0x3F;
        if (type == kAggregation) {
            m_inFragment = false;
            for (size_t at = 2; at + 2 <= size;) {
                size_t length = size_t(p[at] << 8 | p[at + 1]);
                at += 2;
                if (length == 0 || at + length > size) {
                    m_broken = true;
                    return;
                }
                Append(p + at, length);
                at += length;
            }
        } else if (type == kFragmentation) {
            bool start = p[2] & 0x80;
            bool last = p[2] & 0x40;
            if (start) {
                uint8_t header[2] = {uint8_t((p[0] & 0x81) | ((p[2] & 0x3F) << 1)), p[1]};
                m_frame.insert(m_frame.end(), kStartCode, kStartCode + 4);
                m_frame.insert(m_frame.end(), header, header + 2);
                m_inFragment = true;
            } else if (!m_inFragment) {
                m_broken = true;  // its start was lost
                return;
            }
            m_frame.insert(m_frame.end(), p + 3, p + size);
            m_inFragment = !last;
        } else {
            m_inFragment = false;
            Append(p, size);
        }
        return;
    }

    uint8_t type = p[0] & 0x1F;
    if (type == kStapA) {
        m_inFragment = false;
        for (size_t at = 1; at + 2 <= size;) {
            size_t length = size_t(p[at] << 8 | p[at + 1]);
            at += 2;
            if (length == 0 || at + length > size) {
                m_broken = true;
                return;
            }
            Append(p + at, length);
            at += length;
        }
    } else if (type == kFuA) {
        if (size < 2) {
            return;
        }
        bool start = p[1] & 0x80;
        bool last = p[1] & 0x40;
        if (start) {
            uint8_t header = uint8_t((p[0] & 0xE0) | (p[1] & 0x1F));
            m_frame.insert(m_frame.end(), kStartCode, kStartCode + 4);
            m_frame.push_back(header);
            m_inFragment = true;
        } else if (!m_inFragment) {
            m_broken = true;
            return;
        }
        m_frame.insert(m_frame.end(), p + 2, p + size);
        m_inFragment = !last;
    } else if (type >= 1 && type <= 23) {
        m_inFragment = false;
        Append(p, size);
    }
}

void RtpDepacketizer::Append(const uint8_t* nal, size_t size) {
    m_frame.insert(m_frame.end(), kStartCode, kStartCode + 4);
    m_frame.insert(m_frame.end(), nal, nal + size);
}

void RtpDepacketizer::Finish() {
    bool whole = !m_broken && !m_inFragment;
    if (!whole) {
        m_waitingForKeyFrame = true;
        m_onKeyFrameNeeded();
    } else if (!m_frame.empty()) {
        bool key = ContainsKeyFrame(m_codec, m_frame.data(), m_frame.size());
        if (m_waitingForKeyFrame && !key) {
            m_onKeyFrameNeeded();  // still waiting: in case the last request was lost
        } else {
            m_waitingForKeyFrame = false;
            AccessUnit frame;
            frame.codec = m_codec;
            frame.keyFrame = key;
            frame.timestamp = m_timestamp;
            frame.data.swap(m_frame);
            m_onFrame(std::move(frame));
        }
    }
    m_frame.clear();
    m_broken = false;
    m_inFragment = false;
}

RtpDepacketizer::Reception RtpDepacketizer::TakeReception() {
    Reception reception;
    reception.ssrc = m_ssrc;
    reception.highestSequence = m_cycles | m_maxSequence;
    uint32_t expected = reception.highestSequence - m_baseSequence + 1;
    reception.cumulativeLost = expected > m_received ? expected - m_received : 0;
    uint32_t expectedLately = expected - m_expectedAtReport;
    uint32_t receivedLately = m_received - m_receivedAtReport;
    if (expectedLately > receivedLately) {
        reception.fractionLost = uint8_t(((expectedLately - receivedLately) << 8) / expectedLately);
    }
    m_expectedAtReport = expected;
    m_receivedAtReport = m_received;
    return reception;
}

std::vector<uint8_t> BuildReceiverReport(uint32_t ourSsrc,
                                         const RtpDepacketizer::Reception& reception) {
    std::vector<uint8_t> report(32, 0);
    auto put32 = [&](size_t at, uint32_t value) {
        for (int i = 0; i < 4; ++i) report[at + size_t(i)] = uint8_t(value >> (24 - 8 * i));
    };
    report[0] = 0x81;  // version 2, one report block
    report[1] = 201;   // receiver report
    report[3] = 7;     // length in words, less one
    put32(4, ourSsrc);
    put32(8, reception.ssrc);
    uint32_t lost = reception.cumulativeLost > 0x7FFFFF ? 0x7FFFFF : reception.cumulativeLost;
    put32(12, uint32_t(reception.fractionLost) << 24 | lost);
    put32(16, reception.highestSequence);
    // Jitter, last sender report and delay since: none kept.
    return report;
}

std::vector<uint8_t> BuildPictureLossIndication(uint32_t ourSsrc, uint32_t mediaSsrc) {
    std::vector<uint8_t> pli = {0x81, 206, 0, 2};  // version 2, PLI; payload feedback; 3 words
    for (uint32_t ssrc : {ourSsrc, mediaSsrc}) {
        for (int i = 0; i < 4; ++i) pli.push_back(uint8_t(ssrc >> (24 - 8 * i)));
    }
    return pli;
}

}  // namespace gazeora
