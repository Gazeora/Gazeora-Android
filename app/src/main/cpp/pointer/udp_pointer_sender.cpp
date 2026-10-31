// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "pointer/udp_pointer_sender.h"

#include <android/log.h>
#include <sys/socket.h>

#include <cerrno>

namespace gazeora {

namespace {

constexpr char kTag[] = "GazeoraPointer";

}  // namespace

void UdpPointerSender::Send(const PointerEvent& event) {
    if (!Connect()) {
        return;
    }
    std::vector<uint8_t> message = PointerMessage(event);
    send(m_fd.Get(), message.data(), message.size(), MSG_NOSIGNAL);
}

bool UdpPointerSender::Connect() {
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
        m_failed = true;  // touches stay on the phone; the stream goes on
        __android_log_print(ANDROID_LOG_WARN, kTag, "no touch input: %s", error.c_str());
    }
    return bool(m_fd);
}

}  // namespace gazeora
