// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <netinet/in.h>

#include <cstddef>
#include <cstdint>
#include <string>

namespace gazeora::net {

// A socket or file descriptor, closed when it goes.
class Fd {
public:
    Fd() = default;
    explicit Fd(int fd) : m_fd(fd) {}
    Fd(Fd&& other) noexcept : m_fd(other.Release()) {}
    Fd& operator=(Fd&& other) noexcept {
        Reset(other.Release());
        return *this;
    }
    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;
    ~Fd() { Reset(); }

    int Get() const { return m_fd; }
    explicit operator bool() const { return m_fd >= 0; }
    int Release() {
        int fd = m_fd;
        m_fd = -1;
        return fd;
    }
    void Reset(int fd = -1);

private:
    int m_fd = -1;
};

// The IPv4 address of `host`, a dotted address or a name. False with `error` set if it has
// none.
bool Resolve(const std::string& host, uint16_t port, sockaddr_in& out, std::string& error);

// A TCP connection to `address`, giving up after `timeoutMs`. Empty with `error` set if it
// can't.
Fd ConnectTcp(const sockaddr_in& address, int timeoutMs, std::string& error);

// Sends all of `data`, for up to `timeoutMs`; false if the connection failed or stalled.
bool SendAll(int fd, const void* data, size_t size, int timeoutMs = 5000);

// Waits up to `timeoutMs` for `fd` to have something to read (or to fail).
bool WaitReadable(int fd, int timeoutMs);

// A UDP socket bound to `port` on every interface (0: any free port).
Fd BindUdp(uint16_t port, std::string& error);

// The port a socket is bound to.
uint16_t LocalPort(int fd);

std::string ErrorText(int code);

// Milliseconds on a steady clock.
uint64_t NowMs();

}  // namespace gazeora::net
