// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "protocols/frame_assembler.h"

#include <iterator>

namespace gazeora {

namespace {

// How long an unfinished frame waits for its late chunks once a later frame has started.
constexpr uint64_t kReorderWaitMs = 10;
// A frame this far behind isn't late: the PC restarted (a protocol or codec change) and
// numbers its frames from 0 again.
constexpr int32_t kRestartedBehind = 64;

}  // namespace

void FrameAssembler::Add(const GzfHeader& header, const uint8_t* payload, size_t size,
                         uint64_t nowMs) {
    uint32_t count = header.tail & 0xFFFF;
    uint32_t index = header.tail >> 16;
    if (count == 0 || index >= count) {
        return;
    }
    if (m_counted && int32_t(header.frameNumber - m_lastCounted) < -kRestartedBehind) {
        Reset();  // far behind: the PC's stream started over, numbering from 0
    }
    if (m_counted && int32_t(header.frameNumber - m_lastCounted) <= 0) {
        return;  // a chunk of a frame already done with
    }
    if (m_partials.empty()) {
        m_base = header.frameNumber;
    }
    int32_t key = int32_t(header.frameNumber - m_base);
    auto it = m_partials.find(key);
    if (it == m_partials.end()) {
        Partial partial;
        partial.header = header;
        partial.chunks.assign(count, {});
        partial.have.assign(count, false);
        partial.startedMs = nowMs;
        it = m_partials.emplace(key, std::move(partial)).first;
    }
    Partial& frame = it->second;
    if (frame.chunks.size() != count || frame.have[index]) {
        return;
    }
    frame.chunks[index].assign(payload, payload + size);
    frame.have[index] = true;
    ++frame.received;
    Settle(nowMs);
}

void FrameAssembler::Tick(uint64_t nowMs) { Settle(nowMs); }

void FrameAssembler::Settle(uint64_t nowMs) {
    while (!m_partials.empty()) {
        auto earliest = m_partials.begin();
        uint32_t frameNumber = earliest->second.header.frameNumber;
        if (earliest->second.Whole()) {
            Partial frame = std::move(earliest->second);
            m_partials.erase(earliest);
            Finish(frame);
            continue;
        }
        // Unfinished, with a later frame arriving for a while or one two on started: its
        // missing chunks aren't just late.
        auto next = std::next(earliest);
        bool overtaken = next != m_partials.end() &&
                         (nowMs - next->second.startedMs >= kReorderWaitMs ||
                          m_partials.rbegin()->first - earliest->first >= 2);
        if (!overtaken) {
            return;
        }
        m_partials.erase(earliest);
        Lose(frameNumber);
    }
}

int FrameAssembler::TakeLossPerMille() {
    int total = m_framesLost + m_framesDone;
    int loss = total == 0 ? 0 : m_framesLost * 1000 / total;
    m_framesLost = m_framesDone = 0;
    return loss;
}

void FrameAssembler::Reset() {
    m_partials.clear();
    m_counted = false;
    m_waitingForKeyFrame = true;
}

void FrameAssembler::CountMissing(uint32_t frameNumber) {
    if (m_counted && int32_t(frameNumber - m_lastCounted) > 1) {
        m_framesLost += int32_t(frameNumber - m_lastCounted) - 1;
        m_waitingForKeyFrame = true;
    }
    m_lastCounted = frameNumber;
    m_counted = true;
}

void FrameAssembler::Lose(uint32_t frameNumber) {
    CountMissing(frameNumber);
    ++m_framesLost;
    m_waitingForKeyFrame = true;
    m_onKeyFrameNeeded();
}

void FrameAssembler::Finish(Partial& partial) {
    CountMissing(partial.header.frameNumber);
    ++m_framesDone;
    if (m_waitingForKeyFrame && !partial.header.KeyFrame()) {
        m_onKeyFrameNeeded();
        return;
    }
    m_waitingForKeyFrame = false;

    AccessUnit frame;
    size_t size = 0;
    for (const auto& chunk : partial.chunks) size += chunk.size();
    frame.data.reserve(size);
    for (const auto& chunk : partial.chunks) {
        frame.data.insert(frame.data.end(), chunk.begin(), chunk.end());
    }
    frame.codec = partial.header.H265() ? VideoCodec::H265 : VideoCodec::H264;
    frame.keyFrame = partial.header.KeyFrame();
    frame.timestamp = partial.header.timestamp;
    frame.width = partial.header.width;
    frame.height = partial.header.height;
    m_onFrame(std::move(frame));
}

}  // namespace gazeora
