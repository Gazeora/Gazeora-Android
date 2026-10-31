// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "sources/udp_source.h"

#include <sys/socket.h>

#include <cerrno>

#include "net.h"
#include "protocols/frame_assembler.h"
#include "protocols/gzf.h"

namespace gazeora {

namespace {

// Without a frame for this long, the player says it can't hear the PC.
constexpr uint64_t kQuietAfterMs = 4000;
// Key frame requests at most this often: one is on its way.
constexpr uint64_t kKeyFrameRequestEveryMs = 300;
constexpr int kReceiveBufferBytes = 4 << 20;

void Say(int fd, GzfType type, int lossPerMille, uint32_t cookie) {
    auto message = GzfMessage(type, lossPerMille, cookie);
    // Lost like any datagram; the next hello goes out in a second.
    send(fd, message.data(), message.size(), MSG_NOSIGNAL);
}

}  // namespace

void UdpSource::Start() {
    m_running = true;
    m_thread = std::thread([this] { Run(); });
}

void UdpSource::Stop() {
    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void UdpSource::Run() {
    std::string error;
    sockaddr_in pc;
    net::Fd fd;
    if (net::Resolve(m_host, m_port, pc, error)) {
        fd = net::BindUdp(0, error);
    }
    // Only the PC's datagrams; and an ICMP "port unreachable" says it isn't listening.
    if (fd && connect(fd.Get(), reinterpret_cast<sockaddr*>(&pc), sizeof(pc)) != 0) {
        error = net::ErrorText(errno);
        fd.Reset();
    }
    if (!fd) {
        m_sink.OnStatus(PlayerStatus::Failed, "UDP failed: " + error);
        return;
    }
    setsockopt(fd.Get(), SOL_SOCKET, SO_RCVBUF, &kReceiveBufferBytes, sizeof(kReceiveBufferBytes));

    FrameAssembler assembler([this](AccessUnit&& frame) { m_sink.OnFrame(std::move(frame)); },
                             [this] { m_keyFrameWanted = true; });
    uint8_t datagram[kGzfMaxDatagram + 100];
    uint64_t nextHelloMs = 0, lastKeyFrameRequestMs = 0;
    uint64_t lastFrameMs = net::NowMs();
    bool quiet = false;
    uint32_t cookie = 0;  // from the PC's challenge
    while (m_running) {
        uint64_t now = net::NowMs();
        if (now >= nextHelloMs) {
            Say(fd.Get(), GzfType::Hello, assembler.TakeLossPerMille(), cookie);
            nextHelloMs = now + kGzfHelloEveryMs;
        }
        if (now - lastKeyFrameRequestMs >= kKeyFrameRequestEveryMs && m_keyFrameWanted.exchange(false)) {
            Say(fd.Get(), GzfType::KeyFrameRequest, 0, cookie);
            lastKeyFrameRequestMs = now;
        }
        // Every datagram waiting, then back to the hellos; often while a frame waits for
        // chunks that came out of order.
        if (net::WaitReadable(fd.Get(), assembler.Waiting() ? 2 : 100)) {
            ssize_t got;
            while ((got = recv(fd.Get(), datagram, sizeof(datagram), MSG_DONTWAIT)) >= 0 ||
                   errno == ECONNREFUSED) {
                // Refused: nothing listening yet. Keep saying hello.
                GzfHeader header;
                if (got <= 0 || !GzfHeader::Read(datagram, size_t(got), header)) {
                    continue;
                }
                if (header.type == uint8_t(GzfType::Challenge)) {
                    // Hello again with its cookie right away, so the frames start.
                    cookie = header.timestamp;
                    Say(fd.Get(), GzfType::Hello, 0, cookie);
                    nextHelloMs = net::NowMs() + kGzfHelloEveryMs;
                } else if (header.type == uint8_t(GzfType::Frame)) {
                    assembler.Add(header, datagram + kGzfHeaderSize, size_t(got) - kGzfHeaderSize,
                                  net::NowMs());
                    lastFrameMs = net::NowMs();
                    if (quiet) {
                        quiet = false;
                        m_sink.OnStatus(PlayerStatus::Playing, {});
                    }
                }
            }
        }
        assembler.Tick(net::NowMs());
        if (!quiet && net::NowMs() - lastFrameMs > kQuietAfterMs) {
            quiet = true;
            m_sink.OnStatus(PlayerStatus::NoFrames, {});
        }
    }
    Say(fd.Get(), GzfType::Bye, 0, cookie);
}

}  // namespace gazeora
