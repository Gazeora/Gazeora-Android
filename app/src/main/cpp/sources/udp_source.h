// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <atomic>
#include <string>
#include <thread>

#include "sources/stream_source.h"

namespace gazeora {

// Frames from the PC over UDP (Gazeora's UDP protocol). The player says hello once a second,
// which is how the PC knows where to send; the frames come back the same way, through the
// Android emulator's NAT too.
class UdpSource : public StreamSource {
public:
    UdpSource(std::string host, uint16_t port, SourceSink& sink)
        : m_host(std::move(host)), m_port(port), m_sink(sink) {}
    ~UdpSource() override { Stop(); }

    void Start() override;
    void Stop() override;
    void RequestKeyFrame() override { m_keyFrameWanted = true; }

private:
    void Run();

    std::string m_host;
    uint16_t m_port;
    SourceSink& m_sink;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_keyFrameWanted{true};
    std::thread m_thread;
};

}  // namespace gazeora
