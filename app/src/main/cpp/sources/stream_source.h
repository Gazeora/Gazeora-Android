// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <string>

#include "protocols/access_unit.h"

namespace gazeora {

// How playing is going, for the screen. The numbers are NativePlayer.java's STATUS_*.
enum class PlayerStatus {
    Connecting = 0,
    Playing = 1,         // frames are arriving (again)
    NoFrames = 2,        // connected, but nothing has arrived for a while
    Failed = 3,          // detail says why; playing has stopped
    RtspUdpBlocked = 4,  // RTSP over UDP: set up, but no video gets through
    UsbClosed = 5,       // the cable went, or the PC let go: the app waits for it again
};

class SourceSink {
public:
    virtual ~SourceSink() = default;
    // From the source's thread.
    virtual void OnFrame(AccessUnit&& frame) = 0;
    virtual void OnStatus(PlayerStatus status, const std::string& detail) = 0;
};

// Where frames come from: one per protocol. Runs on its own threads between Start and Stop.
class StreamSource {
public:
    virtual ~StreamSource() = default;
    virtual void Start() = 0;
    // Waits for its threads; no calls to the sink after.
    virtual void Stop() = 0;
    // The decoder lost its place. Protocols without a way to ask wait for the next one.
    virtual void RequestKeyFrame() {}
};

}  // namespace gazeora
