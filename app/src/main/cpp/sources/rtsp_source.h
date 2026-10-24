#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "net.h"
#include "protocols/rtp_depacketizer.h"
#include "protocols/rtsp_message.h"
#include "sources/stream_source.h"

namespace gazeora {

// Plays the PC's RTSP stream (Gazeora's RTSP protocols): DESCRIBE, SETUP, PLAY, then RTP
// either interleaved in the RTSP connection (TCP) or on a pair of UDP ports, with receiver
// reports so the PC can adjust the bitrate. A GET_PARAMETER keeps the session alive.
class RtspSource : public StreamSource {
public:
    RtspSource(std::string host, uint16_t port, std::string path, bool tcp, SourceSink& sink)
        : m_host(std::move(host)), m_port(port), m_path(std::move(path)), m_tcp(tcp),
          m_sink(sink) {}
    ~RtspSource() override { Stop(); }

    void Start() override;
    void Stop() override;
    // Over UDP, a Picture Loss Indication tells the PC; over TCP nothing is lost.
    void RequestKeyFrame() override { m_keyFrameWanted = true; }

private:
    void Run();
    // Sets up and starts the session; false with `error` set if it can't.
    bool Open(std::string& error);
    bool OpenUdpPorts(std::string& error);
    void Play();
    // Sends a request and, unless `wait` is false, waits for its response (taking interleaved
    // data that comes first).
    bool Request(const std::string& method, const std::string& url, const std::string& headers,
                 Response* response, std::string& error, bool wait = true);
    // Reads what has arrived on the RTSP connection; false if it closed.
    bool ReadConnection(int timeoutMs);
    // Takes whole messages out of the connection's buffer.
    void TakeMessages();
    // A receiver report, and a key frame request after it if `askForKeyFrame`.
    void SendReceiverReport(bool askForKeyFrame);
    // Whether an RTP datagram came from the PC: its address, and its RTP port if it said one.
    bool FromPc(const sockaddr_in& from) const;

    std::string m_host;
    uint16_t m_port;
    std::string m_path;
    bool m_tcp;
    SourceSink& m_sink;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_keyFrameWanted{false};
    std::thread m_thread;

    // The session thread's.
    sockaddr_in m_pc = {};
    net::Fd m_rtsp;
    net::Fd m_rtp;
    net::Fd m_rtcp;
    sockaddr_in m_serverRtcp = {};
    bool m_haveServerRtcp = false;
    uint16_t m_serverRtpPort = 0;  // where the PC sends RTP from; 0 if it didn't say
    std::string m_url;
    std::string m_session;
    int m_timeoutSeconds = 60;
    int m_cseq = 0;
    int m_channel = 0;
    uint32_t m_ssrc = 0;
    uint64_t m_packets = 0;  // RTP packets received
    std::vector<uint8_t> m_inbox;
    std::vector<Response> m_responses;
    std::unique_ptr<RtpDepacketizer> m_depacketizer;
};

}  // namespace gazeora
