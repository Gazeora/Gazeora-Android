#pragma once

#include <string>

#include "net.h"
#include "pointer/pointer_sender.h"

namespace gazeora {

// Touches over the network: a datagram each to the PC's pointer port, whatever protocol
// the frames come over. A lost one is a missed move; the next one puts the pointer right.
class UdpPointerSender final : public PointerSender {
public:
    explicit UdpPointerSender(std::string host) : m_host(std::move(host)) {}

    void Send(const PointerEvent& event) override;

private:
    // Resolves the PC on first use, as a name lookup can take a while.
    bool Connect();

    std::string m_host;
    net::Fd m_fd;
    bool m_failed = false;
};

}  // namespace gazeora
