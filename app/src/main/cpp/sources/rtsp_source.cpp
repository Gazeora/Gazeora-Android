#include "sources/rtsp_source.h"

#include <poll.h>
#include <sys/socket.h>

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <random>

namespace gazeora {

namespace {

constexpr int kConnectTimeoutMs = 5000;
constexpr int kResponseTimeoutMs = 5000;
// Without video for this long the player says so; over UDP, when none ever came, it gives up:
// something between the PC and this phone drops it (the Android emulator does).
constexpr uint64_t kQuietAfterMs = 4000;
constexpr uint64_t kReportEveryMs = 1000;
// Key frame requests at most this often: one is on its way.
constexpr uint64_t kKeyFrameRequestEveryMs = 300;
constexpr int kReceiveBufferBytes = 4 << 20;
constexpr size_t kMaxInbox = 8 << 20;

std::string StatusText(const Response& response) {
    return std::to_string(response.status) + " " + response.reason;
}

}  // namespace

void RtspSource::Start() {
    m_running = true;
    m_thread = std::thread([this] { Run(); });
}

void RtspSource::Stop() {
    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void RtspSource::Run() {
    std::string error;
    if (!Open(error)) {
        if (m_running) {
            m_sink.OnStatus(PlayerStatus::Failed, error);
        }
        return;
    }
    Play();
    std::string ignored;
    Request("TEARDOWN", m_url, {}, nullptr, ignored, false);
}

bool RtspSource::Open(std::string& error) {
    if (!net::Resolve(m_host, m_port, m_pc, error)) {
        return false;
    }
    m_rtsp = net::ConnectTcp(m_pc, kConnectTimeoutMs, error);
    if (!m_rtsp) {
        return false;
    }
    m_url = "rtsp://" + m_host + ":" + std::to_string(m_port) + (m_path.empty() ? "/" : m_path);

    Response describe;
    if (!Request("DESCRIBE", m_url, "Accept: application/sdp\r\n", &describe, error)) {
        return false;
    }
    if (describe.status != 200) {
        error = "The PC answered " + StatusText(describe);
        return false;
    }
    SdpVideo video;
    if (!ParseSdpVideo(describe.body, video)) {
        error = "The stream has no H.264 or H.265 video";
        return false;
    }
    std::string base = describe.Header("Content-Base");
    if (base.empty()) {
        base = describe.Header("Content-Location");
    }
    if (base.empty()) {
        base = m_url;
    }
    m_depacketizer = std::make_unique<RtpDepacketizer>(
        video.codec, [this](AccessUnit&& frame) { m_sink.OnFrame(std::move(frame)); },
        [this] { m_keyFrameWanted = true; });

    std::string transport;
    if (m_tcp) {
        transport = "RTP/AVP/TCP;unicast;interleaved=0-1";
    } else {
        if (!OpenUdpPorts(error)) {
            return false;
        }
        uint16_t rtpPort = net::LocalPort(m_rtp.Get());
        transport = "RTP/AVP;unicast;client_port=" + std::to_string(rtpPort) + "-" +
                    std::to_string(rtpPort + 1);
    }
    Response setup;
    if (!Request("SETUP", ResolveControl(base, video.control), "Transport: " + transport + "\r\n",
                 &setup, error)) {
        return false;
    }
    if (setup.status == 461) {
        error = std::string("The PC refused RTP over ") + (m_tcp ? "TCP" : "UDP") +
                ": it streams with another protocol. Pick the one Gazeora on the PC is set to.";
        return false;
    }
    if (setup.status != 200) {
        error = "The PC answered SETUP with " + StatusText(setup);
        return false;
    }
    m_session = SessionId(setup.Header("Session"));
    m_timeoutSeconds = SessionTimeoutSeconds(setup.Header("Session"));
    std::string replied = setup.Header("Transport");
    if (m_tcp) {
        std::string channels = TransportParameter(replied, "interleaved");
        if (!channels.empty()) {
            m_channel = std::atoi(channels.c_str());
        }
    } else {
        std::string ports = TransportParameter(replied, "server_port");
        int rtpPort = std::atoi(ports.c_str());
        if (rtpPort > 0 && rtpPort <= 65535) {
            m_serverRtpPort = uint16_t(rtpPort);
        }
        size_t dash = ports.find('-');
        if (dash != std::string::npos) {
            m_serverRtcp = m_pc;
            m_serverRtcp.sin_port = htons(uint16_t(std::atoi(ports.c_str() + dash + 1)));
            m_haveServerRtcp = true;
        }
    }

    Response play;
    if (!Request("PLAY", m_url, "Range: npt=0.000-\r\n", &play, error)) {
        return false;
    }
    if (play.status != 200) {
        error = "The PC answered PLAY with " + StatusText(play);
        return false;
    }
    return true;
}

bool RtspSource::OpenUdpPorts(std::string& error) {
    // RTP on an even port, RTCP on the next one.
    for (int attempt = 0; attempt < 20; ++attempt) {
        net::Fd rtp = net::BindUdp(0, error);
        if (!rtp) {
            return false;
        }
        uint16_t port = net::LocalPort(rtp.Get());
        if (port % 2 != 0) {
            continue;
        }
        net::Fd rtcp = net::BindUdp(uint16_t(port + 1), error);
        if (!rtcp) {
            continue;
        }
        setsockopt(rtp.Get(), SOL_SOCKET, SO_RCVBUF, &kReceiveBufferBytes,
                   sizeof(kReceiveBufferBytes));
        m_rtp = std::move(rtp);
        m_rtcp = std::move(rtcp);
        return true;
    }
    error = "No pair of UDP ports free for RTP";
    return false;
}

void RtspSource::Play() {
    std::mt19937 random{std::random_device()()};
    m_ssrc = random();
    const uint64_t startMs = net::NowMs();
    uint64_t nextKeepAliveMs = startMs + uint64_t(m_timeoutSeconds) * 1000 / 2;
    uint64_t nextReportMs = startMs + kReportEveryMs;
    uint64_t lastKeyFrameRequestMs = 0;
    uint64_t lastVideoMs = startMs;
    bool gotVideo = false, quiet = false;
    uint8_t datagram[2048];

    while (m_running) {
        const uint64_t packetsBefore = m_packets;
        if (m_tcp) {
            if (!ReadConnection(100)) {
                m_sink.OnStatus(PlayerStatus::Failed, "The PC closed the stream");
                return;
            }
        } else {
            // Often while packets wait for one that came out of order.
            pollfd fds[2] = {{m_rtp.Get(), POLLIN, 0}, {m_rtsp.Get(), POLLIN, 0}};
            if (poll(fds, 2, m_depacketizer->Waiting() ? 2 : 100) > 0) {
                if (fds[0].revents & POLLIN) {
                    sockaddr_in from = {};
                    socklen_t fromLength = sizeof(from);
                    ssize_t got;
                    while ((got = recvfrom(m_rtp.Get(), datagram, sizeof(datagram), MSG_DONTWAIT,
                                           reinterpret_cast<sockaddr*>(&from), &fromLength)) > 0) {
                        fromLength = sizeof(from);
                        // Only the PC's: anyone else on the network could put pictures in, or
                        // with a new SSRC start the stream over.
                        if (!FromPc(from)) {
                            continue;
                        }
                        m_depacketizer->Add(datagram, size_t(got), net::NowMs());
                        ++m_packets;
                    }
                }
                if ((fds[1].revents & (POLLIN | POLLHUP | POLLERR)) && !ReadConnection(0)) {
                    m_sink.OnStatus(PlayerStatus::Failed, "The PC closed the stream");
                    return;
                }
            }
        }
        m_responses.clear();  // keep-alives' answers

        uint64_t now = net::NowMs();
        m_depacketizer->Tick(now);
        if (m_packets != packetsBefore) {
            lastVideoMs = now;
            gotVideo = true;
            if (quiet) {
                quiet = false;
                m_sink.OnStatus(PlayerStatus::Playing, {});
            }
        } else if (!quiet && now - lastVideoMs > kQuietAfterMs) {
            if (!m_tcp && !gotVideo) {
                m_sink.OnStatus(PlayerStatus::RtspUdpBlocked, {});
                return;
            }
            quiet = true;
            m_sink.OnStatus(PlayerStatus::NoFrames, {});
        }
        if (now >= nextKeepAliveMs) {
            std::string ignored;
            Request("GET_PARAMETER", m_url, {}, nullptr, ignored, false);
            nextKeepAliveMs = now + uint64_t(m_timeoutSeconds) * 1000 / 2;
        }
        if (!m_tcp) {
            bool askNow = now - lastKeyFrameRequestMs >= kKeyFrameRequestEveryMs &&
                          m_keyFrameWanted.exchange(false);
            if (askNow || now >= nextReportMs) {
                SendReceiverReport(askNow);
                nextReportMs = now + kReportEveryMs;
                if (askNow) {
                    lastKeyFrameRequestMs = now;
                }
            }
        }
    }
}

bool RtspSource::Request(const std::string& method, const std::string& url,
                         const std::string& headers, Response* response, std::string& error,
                         bool wait) {
    std::string request = method + " " + url + " RTSP/1.0\r\nCSeq: " + std::to_string(++m_cseq) +
                          "\r\nUser-Agent: Gazeora Android\r\n";
    if (!m_session.empty()) {
        request += "Session: " + m_session + "\r\n";
    }
    request += headers + "\r\n";
    if (!net::SendAll(m_rtsp.Get(), request.data(), request.size())) {
        error = "The connection to the PC failed";
        return false;
    }
    if (!wait) {
        return true;
    }
    m_responses.clear();
    uint64_t giveUpAt = net::NowMs() + kResponseTimeoutMs;
    while (m_responses.empty()) {
        if (!m_running) {
            error = "Stopped";
            return false;
        }
        uint64_t now = net::NowMs();
        if (now >= giveUpAt) {
            error = "The PC didn't answer " + method;
            return false;
        }
        if (!ReadConnection(int(std::min<uint64_t>(giveUpAt - now, 100)))) {
            error = "The PC closed the connection";
            return false;
        }
    }
    *response = std::move(m_responses.front());
    m_responses.clear();
    return true;
}

bool RtspSource::ReadConnection(int timeoutMs) {
    if (!net::WaitReadable(m_rtsp.Get(), timeoutMs)) {
        return true;
    }
    uint8_t chunk[64 * 1024];
    while (true) {
        ssize_t got = recv(m_rtsp.Get(), chunk, sizeof(chunk), MSG_DONTWAIT);
        if (got == 0) {
            return false;
        }
        if (got < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                break;
            }
            return false;
        }
        m_inbox.insert(m_inbox.end(), chunk, chunk + got);
        TakeMessages();
        if (size_t(got) < sizeof(chunk)) {
            break;
        }
    }
    return true;
}

void RtspSource::TakeMessages() {
    size_t at = 0;
    while (at < m_inbox.size()) {
        const uint8_t* p = m_inbox.data() + at;
        size_t left = m_inbox.size() - at;
        if (p[0] == '$') {
            // '$', channel, length (2), then an RTP or RTCP packet.
            if (left < 4) {
                break;
            }
            size_t length = size_t(p[2] << 8 | p[3]);
            if (left < 4 + length) {
                break;
            }
            if (p[1] == m_channel && m_depacketizer) {
                m_depacketizer->Add(p + 4, length, net::NowMs());
                ++m_packets;
            }
            at += 4 + length;
        } else if (p[0] == 'R') {
            Response response;
            long taken = ParseResponse(reinterpret_cast<const char*>(p), left, true, response);
            if (taken == 0) {
                break;
            }
            if (taken < 0) {
                ++at;  // not a response after all
                continue;
            }
            m_responses.push_back(std::move(response));
            at += size_t(taken);
        } else {
            ++at;  // out of step: find the next message
        }
    }
    m_inbox.erase(m_inbox.begin(), m_inbox.begin() + long(at));
    if (m_inbox.size() > kMaxInbox) {
        m_inbox.clear();
    }
}

bool RtspSource::FromPc(const sockaddr_in& from) const {
    return from.sin_family == AF_INET && from.sin_addr.s_addr == m_pc.sin_addr.s_addr &&
           (m_serverRtpPort == 0 || ntohs(from.sin_port) == m_serverRtpPort);
}

void RtspSource::SendReceiverReport(bool askForKeyFrame) {
    if (!m_haveServerRtcp || !m_depacketizer->Started()) {
        return;
    }
    RtpDepacketizer::Reception reception = m_depacketizer->TakeReception();
    std::vector<uint8_t> report = BuildReceiverReport(m_ssrc, reception);
    if (askForKeyFrame) {
        std::vector<uint8_t> pli = BuildPictureLossIndication(m_ssrc, reception.ssrc);
        report.insert(report.end(), pli.begin(), pli.end());
    }
    sendto(m_rtcp.Get(), report.data(), report.size(), MSG_NOSIGNAL,
           reinterpret_cast<const sockaddr*>(&m_serverRtcp), sizeof(m_serverRtcp));
}

}  // namespace gazeora
