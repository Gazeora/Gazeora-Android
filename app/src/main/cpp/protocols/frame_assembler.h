#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <vector>

#include "protocols/access_unit.h"
#include "protocols/gzf.h"

namespace gazeora {

// Puts GZF1 frames back together from their chunks, and keeps a lost frame from smearing the
// picture: after one, frames are skipped up to the next key frame (which the source asks
// for). Also counts the share of frames lost, which the player reports in its hellos so the
// PC can lower the bitrate. Use from one thread.
//
// Wi-Fi delivers some datagrams out of order, a frame's last chunk after the next frame's
// first, so an unfinished frame is only given up as lost a few milliseconds after a later one
// started arriving, or once one two frames on has.
class FrameAssembler {
public:
    using FrameSink = std::function<void(AccessUnit&&)>;
    // A frame was lost, or skipped for want of a key frame: one should be asked for.
    using KeyFrameNeeded = std::function<void()>;

    FrameAssembler(FrameSink onFrame, KeyFrameNeeded onKeyFrameNeeded)
        : m_onFrame(std::move(onFrame)), m_onKeyFrameNeeded(std::move(onKeyFrameNeeded)) {}

    // One chunk, arriving at `nowMs` (any steady clock); header.tail holds its index (high 16
    // bits) and the chunk count.
    void Add(const GzfHeader& header, const uint8_t* payload, size_t size, uint64_t nowMs = 0);
    // Gives up on frames whose chunks haven't come in time. Call often while Waiting.
    void Tick(uint64_t nowMs);
    // Whether a frame is unfinished with a later one started.
    bool Waiting() const { return m_partials.size() > 1; }
    // The share of frames lost since the last call, per mille.
    int TakeLossPerMille();
    // Forgets the frames being put together, and waits for a key frame.
    void Reset();

private:
    struct Partial {
        GzfHeader header;
        std::vector<std::vector<uint8_t>> chunks;
        std::vector<bool> have;
        uint32_t received = 0;
        uint64_t startedMs = 0;  // its first chunk's arrival

        bool Whole() const { return received == chunks.size(); }
    };

    // Hands over the frames that are ready, in order, and gives up on the ones that won't be.
    void Settle(uint64_t nowMs);
    void Lose(uint32_t frameNumber);
    void Finish(Partial& frame);
    // Frames between the last counted and `frameNumber` that never showed up.
    void CountMissing(uint32_t frameNumber);

    FrameSink m_onFrame;
    KeyFrameNeeded m_onKeyFrameNeeded;
    // By distance from m_base, so the order holds when frame numbers wrap.
    std::map<int32_t, Partial> m_partials;
    uint32_t m_base = 0;
    uint32_t m_lastCounted = 0;  // delivered, skipped or lost
    bool m_counted = false;
    bool m_waitingForKeyFrame = true;
    int m_framesLost = 0;
    int m_framesDone = 0;
};

}  // namespace gazeora
