#pragma once

#include <atomic>
#include <string>
#include <thread>

#include "sources/stream_source.h"

namespace gazeora {

// The PC's live MPEG-TS stream over HTTP (Gazeora's HTTP protocol): one GET, then the
// transport stream until the connection closes.
class HttpSource : public StreamSource {
public:
    HttpSource(std::string host, uint16_t port, std::string path, SourceSink& sink)
        : m_host(std::move(host)), m_port(port), m_path(std::move(path)), m_sink(sink) {}
    ~HttpSource() override { Stop(); }

    void Start() override;
    void Stop() override;

private:
    void Run();

    std::string m_host;
    uint16_t m_port;
    std::string m_path;
    SourceSink& m_sink;
    std::atomic<bool> m_running{false};
    std::thread m_thread;
};

}  // namespace gazeora
