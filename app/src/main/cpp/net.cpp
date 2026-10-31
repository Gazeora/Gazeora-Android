// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "net.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstring>

namespace gazeora::net {

void Fd::Reset(int fd) {
    if (m_fd >= 0) {
        close(m_fd);
    }
    m_fd = fd;
}

bool Resolve(const std::string& host, uint16_t port, sockaddr_in& out, std::string& error) {
    out = {};
    out.sin_family = AF_INET;
    out.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &out.sin_addr) == 1) {
        return true;
    }
    addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* found = nullptr;
    int result = getaddrinfo(host.c_str(), nullptr, &hints, &found);
    if (result != 0 || !found) {
        error = "Can't find " + host + ": " + gai_strerror(result);
        return false;
    }
    out.sin_addr = reinterpret_cast<sockaddr_in*>(found->ai_addr)->sin_addr;
    freeaddrinfo(found);
    return true;
}

Fd ConnectTcp(const sockaddr_in& address, int timeoutMs, std::string& error) {
    Fd fd(socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_TCP));
    if (!fd) {
        error = "socket failed: " + ErrorText(errno);
        return {};
    }
    int flags = fcntl(fd.Get(), F_GETFL);
    fcntl(fd.Get(), F_SETFL, flags | O_NONBLOCK);
    if (connect(fd.Get(), reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0) {
        if (errno != EINPROGRESS) {
            error = ErrorText(errno);
            return {};
        }
        pollfd waiting = {fd.Get(), POLLOUT, 0};
        int ready = poll(&waiting, 1, timeoutMs);
        if (ready == 0) {
            error = "No answer within " + std::to_string(timeoutMs / 1000) + " seconds";
            return {};
        }
        int code = 0;
        socklen_t length = sizeof(code);
        getsockopt(fd.Get(), SOL_SOCKET, SO_ERROR, &code, &length);
        if (ready < 0 || code != 0) {
            error = ErrorText(ready < 0 ? errno : code);
            return {};
        }
    }
    int one = 1;
    setsockopt(fd.Get(), IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    return fd;
}

bool SendAll(int fd, const void* data, size_t size, int timeoutMs) {
    const char* at = static_cast<const char*>(data);
    uint64_t giveUpAt = NowMs() + uint64_t(timeoutMs);
    while (size > 0) {
        ssize_t sent = send(fd, at, size, MSG_NOSIGNAL);
        if (sent > 0) {
            at += sent;
            size -= size_t(sent);
            continue;
        }
        if (sent < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            return false;
        }
        uint64_t now = NowMs();
        if (now >= giveUpAt) {
            return false;
        }
        pollfd waiting = {fd, POLLOUT, 0};
        poll(&waiting, 1, int(giveUpAt - now));
    }
    return true;
}

bool WaitReadable(int fd, int timeoutMs) {
    pollfd waiting = {fd, POLLIN, 0};
    return poll(&waiting, 1, timeoutMs) > 0;
}

Fd BindUdp(uint16_t port, std::string& error) {
    Fd fd(socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_UDP));
    if (!fd) {
        error = "socket failed: " + ErrorText(errno);
        return {};
    }
    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);
    if (bind(fd.Get(), reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        error = "bind failed: " + ErrorText(errno);
        return {};
    }
    return fd;
}

uint16_t LocalPort(int fd) {
    sockaddr_in address = {};
    socklen_t length = sizeof(address);
    getsockname(fd, reinterpret_cast<sockaddr*>(&address), &length);
    return ntohs(address.sin_port);
}

std::string ErrorText(int code) {
    switch (code) {
        case ECONNREFUSED: return "Nothing is listening there (connection refused)";
        case ENETUNREACH:
        case EHOSTUNREACH: return "The PC can't be reached from this network";
        case ETIMEDOUT: return "The connection timed out";
        case EACCES:
        case EPERM: return "Network access was refused";
        default: return std::strerror(code);
    }
}

uint64_t NowMs() {
    return uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now().time_since_epoch())
                        .count());
}

}  // namespace gazeora::net
