#include "sources/http_source.h"

#include <sys/socket.h>

#include <cerrno>
#include <vector>

#include "net.h"
#include "protocols/rtsp_message.h"
#include "protocols/ts_demuxer.h"

namespace gazeora {

namespace {

constexpr int kConnectTimeoutMs = 5000;
constexpr uint64_t kResponseTimeoutMs = 5000;
// Without data for this long the player says so.
constexpr uint64_t kQuietAfterMs = 4000;
constexpr int kReceiveBufferBytes = 4 << 20;

}  // namespace

void HttpSource::Start() {
    m_running = true;
    m_thread = std::thread([this] { Run(); });
}

void HttpSource::Stop() {
    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void HttpSource::Run() {
    std::string error;
    sockaddr_in pc;
    net::Fd fd;
    if (net::Resolve(m_host, m_port, pc, error)) {
        fd = net::ConnectTcp(pc, kConnectTimeoutMs, error);
    }
    std::string path = m_path.empty() ? "/stream.ts" : m_path;
    std::string request = "GET " + path + " HTTP/1.1\r\nHost: " + m_host + ":" +
                          std::to_string(m_port) +
                          "\r\nUser-Agent: Gazeora Android\r\nAccept: video/mp2t\r\n"
                          "Connection: close\r\n\r\n";
    if (fd && !net::SendAll(fd.Get(), request.data(), request.size())) {
        error = "The connection to the PC failed";
        fd.Reset();
    }
    if (!fd) {
        if (m_running) {
            m_sink.OnStatus(PlayerStatus::Failed, error);
        }
        return;
    }
    setsockopt(fd.Get(), SOL_SOCKET, SO_RCVBUF, &kReceiveBufferBytes, sizeof(kReceiveBufferBytes));

    TsDemuxer demuxer([this](AccessUnit&& frame) { m_sink.OnFrame(std::move(frame)); });
    std::vector<uint8_t> chunk(256 * 1024);
    std::vector<uint8_t> head;  // until the response's headers are through
    bool streaming = false, quiet = false;
    const uint64_t startMs = net::NowMs();
    uint64_t lastDataMs = startMs;
    while (m_running) {
        uint64_t now = net::NowMs();
        if (!streaming && now - startMs > kResponseTimeoutMs) {
            m_sink.OnStatus(PlayerStatus::Failed, "The PC didn't answer");
            return;
        }
        if (streaming && !quiet && now - lastDataMs > kQuietAfterMs) {
            quiet = true;
            m_sink.OnStatus(PlayerStatus::NoFrames, {});
        }
        if (!net::WaitReadable(fd.Get(), 100)) {
            continue;
        }
        ssize_t got = recv(fd.Get(), chunk.data(), chunk.size(), MSG_DONTWAIT);
        if (got < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
            continue;
        }
        if (got <= 0) {
            m_sink.OnStatus(PlayerStatus::Failed, "The PC closed the stream");
            return;
        }
        lastDataMs = net::NowMs();
        if (quiet) {
            quiet = false;
            m_sink.OnStatus(PlayerStatus::Playing, {});
        }
        if (streaming) {
            demuxer.Add(chunk.data(), size_t(got));
            continue;
        }
        head.insert(head.end(), chunk.begin(), chunk.begin() + got);
        Response response;
        long taken = ParseResponse(reinterpret_cast<const char*>(head.data()), head.size(), false,
                                   response);
        if (taken == 0) {
            continue;
        }
        if (taken < 0) {
            m_sink.OnStatus(PlayerStatus::Failed, "The PC's answer isn't HTTP");
            return;
        }
        if (response.status != 200) {
            m_sink.OnStatus(PlayerStatus::Failed, "The PC answered " +
                                                      std::to_string(response.status) + " " +
                                                      response.reason);
            return;
        }
        streaming = true;
        demuxer.Add(head.data() + taken, head.size() - size_t(taken));
        head.clear();
    }
}

}  // namespace gazeora
