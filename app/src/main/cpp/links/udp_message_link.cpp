// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "links/udp_message_link.h"

#include <android/log.h>
#include <sys/socket.h>

#include <cerrno>

#include "protocols/pointer_message.h"

namespace gazeora {

namespace {

constexpr char kTag[] = "GazeoraInput";

}  // namespace

void UdpMessageLink::Write(const std::vector<uint8_t>& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (Connect()) {
        send(m_fd.Get(), message.data(), message.size(), MSG_NOSIGNAL);
    }
}

bool UdpMessageLink::Connect() {
    if (m_fd || m_failed) {
        return bool(m_fd);
    }
    std::string error;
    sockaddr_in pc;
    if (net::Resolve(m_host, kPointerPort, pc, error)) {
        m_fd = net::BindUdp(0, error);
    }
    if (m_fd && connect(m_fd.Get(), reinterpret_cast<sockaddr*>(&pc), sizeof(pc)) != 0) {
        error = net::ErrorText(errno);
        m_fd.Reset();
    }
    if (!m_fd) {
        m_failed = true;  // input stays on the phone; the stream goes on
        __android_log_print(ANDROID_LOG_WARN, kTag, "no input to the PC: %s", error.c_str());
    }
    return bool(m_fd);
}

}  // namespace gazeora
