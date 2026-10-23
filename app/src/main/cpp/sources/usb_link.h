#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

#include "protocols/gzf.h"
#include "sources/stream_source.h"

namespace gazeora {

// The USB connection to the PC (Gazeora's USB protocol): the PC switches the phone to USB
// accessory mode, the app opens the accessory and hands its file descriptor over, and GZF1
// messages flow over its bulk endpoints, each frame whole with its length.
//
// One for the app, kept open while the cable is in, for whichever player is watching: a read
// from an accessory can't be interrupted, so a player that stops tells the PC bye rather than
// closing it, and the next one says hello on the same connection.
class UsbLink {
public:
    static UsbLink& Get();

    bool Open() const { return m_open; }
    // Takes over `fd`, the accessory the app opened. Closed at once if one is open already.
    void Attach(int fd);
    // Where frames go; null when nobody watches, which tells the PC bye. Once it returns, the
    // sink it replaced gets no more calls.
    void SetSink(SourceSink* sink);
    void RequestKeyFrame() { m_keyFrameWanted = true; }
    // One whole GZF1 message to the PC; dropped while the cable is out.
    void Write(const std::vector<uint8_t>& message);

private:
    UsbLink() = default;
    void Read(int fd);
    void Talk();
    void Say(GzfType type, int lossPerMille = 0);

    std::mutex m_sinkMutex;
    SourceSink* m_sink = nullptr;
    std::atomic<uint32_t> m_sinkGeneration{0};

    std::mutex m_writeMutex;
    int m_fd = -1;
    std::atomic<bool> m_open{false};
    std::atomic<bool> m_watched{false};
    std::atomic<bool> m_keyFrameWanted{true};
};

// The USB protocol for one player: the link above, while it plays.
class UsbSource : public StreamSource {
public:
    explicit UsbSource(SourceSink& sink) : m_sink(sink) {}
    ~UsbSource() override { Stop(); }

    void Start() override;
    void Stop() override;
    void RequestKeyFrame() override { UsbLink::Get().RequestKeyFrame(); }

private:
    SourceSink& m_sink;
    bool m_started = false;
};

}  // namespace gazeora
