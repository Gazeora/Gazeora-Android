// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "protocols/access_unit.h"

namespace gazeora {

// An RTSP or HTTP response: status line, headers and, for RTSP, the body its Content-Length
// gives.
struct Response {
    int status = 0;
    std::string reason;
    std::vector<std::pair<std::string, std::string>> headers;
    std::string body;

    // The header's value, by case-insensitive name; empty if there's none.
    std::string Header(const std::string& name) const;
};

// Parses a response at the start of `data`. Returns the bytes it took, 0 if it isn't all there
// yet, -1 if it isn't a response. `withBody`: read the Content-Length body too (RTSP); HTTP's
// stream follows the headers instead.
long ParseResponse(const char* data, size_t size, bool withBody, Response& out);

// The session id of a Session header ("12345678;timeout=60" gives "12345678"), and its timeout
// in seconds (60 if it doesn't say).
std::string SessionId(const std::string& header);
int SessionTimeoutSeconds(const std::string& header);

// A Transport header's parameter, e.g. "server_port" in "RTP/AVP;unicast;server_port=5000-5001"
// gives "5000-5001"; empty if it isn't there.
std::string TransportParameter(const std::string& header, const std::string& name);

// The video in a session description.
struct SdpVideo {
    VideoCodec codec = VideoCodec::H264;
    int payloadType = 96;
    std::string control;  // the media's a=control, as written
};
// False if there's no H.264 or H.265 video.
bool ParseSdpVideo(const std::string& sdp, SdpVideo& out);

// The URL to SETUP: `control` against `base` (RFC 2326, C.1.1).
std::string ResolveControl(const std::string& base, const std::string& control);

}  // namespace gazeora
