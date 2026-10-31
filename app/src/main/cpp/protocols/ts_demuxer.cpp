// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "protocols/ts_demuxer.h"

#include "protocols/annex_b.h"

namespace gazeora {

namespace {

constexpr size_t kPacketSize = 188;
constexpr uint8_t kSync = 0x47;
constexpr uint8_t kStreamTypeH264 = 0x1B;
constexpr uint8_t kStreamTypeH265 = 0x24;
// A PES no frame comes near: a stream that never starts another one doesn't get to fill memory.
constexpr size_t kMaxPes = 16 << 20;

}  // namespace

void TsDemuxer::Add(const uint8_t* data, size_t size) {
    // Finish a packet left partial last time.
    if (!m_pending.empty()) {
        size_t take = kPacketSize - m_pending.size();
        if (take > size) {
            take = size;
        }
        m_pending.insert(m_pending.end(), data, data + take);
        data += take;
        size -= take;
        if (m_pending.size() < kPacketSize) {
            return;
        }
        if (m_pending[0] == kSync) {
            Packet(m_pending.data());
        }
        m_pending.clear();
    }
    size_t at = 0;
    while (at < size) {
        if (data[at] != kSync) {
            ++at;  // out of step: find the next packet
            continue;
        }
        if (size - at < kPacketSize) {
            m_pending.assign(data + at, data + size);
            return;
        }
        Packet(data + at);
        at += kPacketSize;
    }
}

void TsDemuxer::Packet(const uint8_t* packet) {
    bool error = packet[1] & 0x80;
    bool unitStart = packet[1] & 0x40;
    int pid = (packet[1] & 0x1F) << 8 | packet[2];
    int adaptation = (packet[3] >> 4) & 0x3;
    int continuity = packet[3] & 0x0F;
    if (error) {
        if (pid == m_videoPid) {
            m_broken = true;
        }
        return;
    }

    size_t at = 4;
    bool randomAccess = false;
    bool ends = false;  // stuffing: the last packet of its PES
    if (adaptation & 0x2) {
        size_t length = packet[4];
        if (5 + length > kPacketSize) {
            return;
        }
        if (length == 0) {
            ends = true;  // the length byte alone is one byte of stuffing
        } else {
            uint8_t flags = packet[5];
            randomAccess = flags & 0x40;
            size_t used = 1;
            if (flags & 0x10) used += 6;  // PCR
            if (flags & 0x08) used += 6;  // OPCR
            if (flags & 0x04) used += 1;  // splice countdown
            if ((flags & 0x02) && 5 + used < kPacketSize) used += 1 + packet[5 + used];  // private
            if ((flags & 0x01) && 5 + used < kPacketSize) used += 1 + packet[5 + used];  // extension
            ends = length > used;
        }
        at = 5 + length;
    }
    const bool payload = adaptation & 0x1;
    const uint8_t* data = packet + at;
    const size_t size = payload ? kPacketSize - at : 0;

    if (pid == 0 || pid == m_pmtPid) {
        if (!payload || !unitStart || size < 1 || size_t(data[0]) + 1 > size) {
            return;  // sections longer than a packet aren't taken
        }
        size_t pointer = data[0];
        if (pid == 0) {
            Pat(data + 1 + pointer, size - 1 - pointer);
        } else {
            Pmt(data + 1 + pointer, size - 1 - pointer);
        }
        return;
    }
    if (pid != m_videoPid) {
        return;
    }
    if (payload) {
        if (m_continuity >= 0 && continuity != ((m_continuity + 1) & 0x0F)) {
            m_broken = true;
        }
        m_continuity = continuity;
    }
    if (unitStart) {
        Flush();
        m_randomAccess = randomAccess;
        m_broken = false;
    } else if (m_pes.empty()) {
        return;  // joined mid-PES
    }
    m_pes.insert(m_pes.end(), data, data + size);
    if (m_pes.size() > kMaxPes) {
        m_pes.clear();  // dropped; the next unit start begins afresh
        return;
    }
    if (ends) {
        Flush();
    }
}

void TsDemuxer::Pat(const uint8_t* section, size_t size) {
    // table id, section length (2), stream id (2), version, section numbers (2), programs, CRC
    if (size < 12 || section[0] != 0x00) {
        return;
    }
    size_t length = size_t((section[1] & 0x0F) << 8 | section[2]);
    size_t end = 3 + length;
    if (end > size || end < 12) {
        return;
    }
    for (size_t at = 8; at + 4 <= end - 4; at += 4) {
        int program = section[at] << 8 | section[at + 1];
        if (program != 0) {  // 0 is the network PID
            m_pmtPid = (section[at + 2] & 0x1F) << 8 | section[at + 3];
            return;
        }
    }
}

void TsDemuxer::Pmt(const uint8_t* section, size_t size) {
    if (size < 16 || section[0] != 0x02) {
        return;
    }
    size_t length = size_t((section[1] & 0x0F) << 8 | section[2]);
    size_t end = 3 + length;
    if (end > size || end < 16) {
        return;
    }
    size_t programInfo = size_t((section[10] & 0x0F) << 8 | section[11]);
    for (size_t at = 12 + programInfo; at + 5 <= end - 4;) {
        uint8_t type = section[at];
        int pid = (section[at + 1] & 0x1F) << 8 | section[at + 2];
        size_t info = size_t((section[at + 3] & 0x0F) << 8 | section[at + 4]);
        if (type == kStreamTypeH264 || type == kStreamTypeH265) {
            VideoCodec codec = type == kStreamTypeH265 ? VideoCodec::H265 : VideoCodec::H264;
            if (pid != m_videoPid || codec != m_codec) {
                m_pes.clear();
                m_continuity = -1;
                m_videoPid = pid;
                m_codec = codec;
            }
            return;
        }
        at += 5 + info;
    }
}

void TsDemuxer::Flush() {
    std::vector<uint8_t> pes;
    pes.swap(m_pes);
    bool broken = m_broken;
    m_broken = false;
    // 00 00 01, stream id, length (2), flags (2), header length, PTS...
    if (broken || pes.size() < 9 || pes[0] != 0 || pes[1] != 0 || pes[2] != 1) {
        return;
    }
    size_t payload = 9 + size_t(pes[8]);
    if (payload >= pes.size()) {
        return;
    }
    AccessUnit frame;
    frame.codec = m_codec;
    if ((pes[7] & 0x80) && pes.size() >= 14) {
        const uint8_t* p = &pes[9];
        uint64_t pts = uint64_t(p[0] & 0x0E) << 29 | uint64_t(p[1]) << 22 |
                       uint64_t(p[2] & 0xFE) << 14 | uint64_t(p[3]) << 7 | uint64_t(p[4]) >> 1;
        frame.timestamp = uint32_t(pts);
    }
    frame.data.assign(pes.begin() + long(payload), pes.end());
    frame.keyFrame = m_randomAccess || ContainsKeyFrame(m_codec, frame.data.data(), frame.data.size());
    m_onFrame(std::move(frame));
}

}  // namespace gazeora
