// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "protocols/rtsp_message.h"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <sstream>

namespace gazeora {

namespace {

bool EqualsIgnoreCase(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

std::string Trim(const std::string& text) {
    size_t from = text.find_first_not_of(" \t");
    if (from == std::string::npos) {
        return {};
    }
    size_t to = text.find_last_not_of(" \t\r");
    return text.substr(from, to - from + 1);
}

}  // namespace

std::string Response::Header(const std::string& name) const {
    for (const auto& header : headers) {
        if (EqualsIgnoreCase(header.first, name)) {
            return header.second;
        }
    }
    return {};
}

long ParseResponse(const char* data, size_t size, bool withBody, Response& out) {
    static const char kRtsp[] = "RTSP/";
    static const char kHttp[] = "HTTP/";
    size_t prefix = size < 5 ? size : 5;
    if (std::memcmp(data, kRtsp, prefix) != 0 && std::memcmp(data, kHttp, prefix) != 0) {
        return -1;
    }
    constexpr size_t kMaxHeaders = 64 * 1024;
    std::string text(data, size < kMaxHeaders ? size : kMaxHeaders);
    size_t headersEnd = text.find("\r\n\r\n");
    if (headersEnd == std::string::npos) {
        return size >= kMaxHeaders ? -1 : 0;
    }
    out = Response();
    std::istringstream lines(text.substr(0, headersEnd));
    std::string line;
    std::getline(lines, line);
    // "RTSP/1.0 200 OK"
    size_t space = line.find(' ');
    if (space == std::string::npos) {
        return -1;
    }
    out.status = std::atoi(line.c_str() + space + 1);
    size_t reason = line.find(' ', space + 1);
    out.reason = reason == std::string::npos ? std::string() : Trim(line.substr(reason + 1));
    while (std::getline(lines, line)) {
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            out.headers.emplace_back(Trim(line.substr(0, colon)), Trim(line.substr(colon + 1)));
        }
    }
    size_t taken = headersEnd + 4;
    if (withBody) {
        long length = std::atol(out.Header("Content-Length").c_str());
        if (length < 0 || length > 1 << 20) {
            return -1;
        }
        if (size - taken < size_t(length)) {
            return 0;
        }
        out.body.assign(data + taken, size_t(length));
        taken += size_t(length);
    }
    return long(taken);
}

std::string SessionId(const std::string& header) {
    return Trim(header.substr(0, header.find(';')));
}

int SessionTimeoutSeconds(const std::string& header) {
    size_t at = header.find("timeout=");
    int seconds = at == std::string::npos ? 0 : std::atoi(header.c_str() + at + 8);
    return seconds > 0 ? seconds : 60;
}

std::string TransportParameter(const std::string& header, const std::string& name) {
    std::istringstream parts(header);
    std::string part;
    while (std::getline(parts, part, ';')) {
        part = Trim(part);
        size_t equals = part.find('=');
        if (EqualsIgnoreCase(part.substr(0, equals), name)) {
            return equals == std::string::npos ? std::string() : part.substr(equals + 1);
        }
    }
    return {};
}

bool ParseSdpVideo(const std::string& sdp, SdpVideo& out) {
    std::istringstream lines(sdp);
    std::string line;
    bool inVideo = false, found = false;
    std::vector<int> payloadTypes;
    while (std::getline(lines, line)) {
        line = Trim(line);
        if (line.compare(0, 2, "m=") == 0) {
            if (found) {
                break;  // the video's section is over
            }
            inVideo = line.compare(0, 8, "m=video ") == 0;
            payloadTypes.clear();
            if (inVideo) {
                // m=video 0 RTP/AVP 96 97
                std::istringstream fields(line.substr(8));
                std::string port, profile;
                int type;
                fields >> port >> profile;
                while (fields >> type) payloadTypes.push_back(type);
            }
            continue;
        }
        if (!inVideo) {
            continue;
        }
        if (line.compare(0, 9, "a=rtpmap:") == 0) {
            // a=rtpmap:96 H264/90000
            int type = std::atoi(line.c_str() + 9);
            size_t space = line.find(' ');
            if (space == std::string::npos || found) {
                continue;
            }
            std::string encoding = line.substr(space + 1, line.find('/', space) - space - 1);
            for (char& c : encoding) c = char(std::toupper(static_cast<unsigned char>(c)));
            if (encoding == "H264" || encoding == "H265") {
                out.codec = encoding == "H265" ? VideoCodec::H265 : VideoCodec::H264;
                out.payloadType = type;
                found = true;
            }
        } else if (line.compare(0, 10, "a=control:") == 0) {
            out.control = line.substr(10);
        }
    }
    return found;
}

std::string ResolveControl(const std::string& base, const std::string& control) {
    if (control.empty() || control == "*") {
        return base;
    }
    if (control.compare(0, 7, "rtsp://") == 0 || control.compare(0, 8, "rtsps://") == 0) {
        return control;
    }
    if (!base.empty() && base.back() == '/') {
        return base + control;
    }
    return base + "/" + control;
}

}  // namespace gazeora
