#include "discovery.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/socket.h>

#include <algorithm>
#include <cstring>

#include "net.h"

namespace gazeora {

namespace {

constexpr uint64_t kAskEveryMs = 1000;
// Many Wi-Fi networks drop broadcasts, so every address on the network is asked too, less
// often: a few hundred tiny datagrams.
constexpr uint64_t kSweepEveryMs = 3000;
constexpr int kMaxSweepPrefix = 22;  // networks of up to 1022 addresses
constexpr uint64_t kForgetAfterMs = 7000;
constexpr char kEmulatorHost[] = "10.0.2.2";

// Every other address on this device's IPv4 networks (Wi-Fi, Ethernet, a hotspot).
std::vector<in_addr> Neighbours() {
    std::vector<in_addr> neighbours;
    ifaddrs* interfaces = nullptr;
    if (getifaddrs(&interfaces) != 0) {
        return neighbours;
    }
    for (ifaddrs* at = interfaces; at; at = at->ifa_next) {
        if (!at->ifa_addr || !at->ifa_netmask || at->ifa_addr->sa_family != AF_INET ||
            !(at->ifa_flags & IFF_UP) || (at->ifa_flags & IFF_LOOPBACK)) {
            continue;
        }
        uint32_t self = ntohl(reinterpret_cast<sockaddr_in*>(at->ifa_addr)->sin_addr.s_addr);
        uint32_t mask = ntohl(reinterpret_cast<sockaddr_in*>(at->ifa_netmask)->sin_addr.s_addr);
        if (mask == 0 || __builtin_popcount(mask) < kMaxSweepPrefix) {
            continue;  // too big to ask everyone
        }
        uint32_t network = self & mask, broadcast = network | ~mask;
        for (uint32_t host = network + 1; host < broadcast; ++host) {
            if (host != self) {
                neighbours.push_back(in_addr{htonl(host)});
            }
        }
    }
    freeifaddrs(interfaces);
    return neighbours;
}

// Everywhere to ask: the broadcast address of every network, and the emulator's host.
std::vector<in_addr> Targets(bool emulator) {
    std::vector<in_addr> targets;
    auto add = [&targets](in_addr address) {
        for (const in_addr& known : targets) {
            if (known.s_addr == address.s_addr) {
                return;
            }
        }
        targets.push_back(address);
    };
    add(in_addr{htonl(INADDR_BROADCAST)});
    ifaddrs* interfaces = nullptr;
    if (getifaddrs(&interfaces) == 0) {
        for (ifaddrs* at = interfaces; at; at = at->ifa_next) {
            if (!at->ifa_addr || at->ifa_addr->sa_family != AF_INET || !(at->ifa_flags & IFF_UP) ||
                (at->ifa_flags & IFF_LOOPBACK) || !(at->ifa_flags & IFF_BROADCAST) ||
                !at->ifa_broadaddr) {
                continue;
            }
            add(reinterpret_cast<sockaddr_in*>(at->ifa_broadaddr)->sin_addr);
        }
        freeifaddrs(interfaces);
    }
    if (emulator) {
        in_addr host;
        inet_pton(AF_INET, kEmulatorHost, &host);
        add(host);
    }
    return targets;
}

bool SamePc(const Discovery::Pc& a, const Discovery::Pc& b) {
    return a.host == b.host && a.reply.name == b.reply.name &&
           a.reply.protocol == b.reply.protocol && a.reply.port == b.reply.port &&
           a.reply.path == b.reply.path && a.reply.codec == b.reply.codec;
}

}  // namespace

void Discovery::Start() {
    if (m_running) {
        return;
    }
    m_running = true;
    m_thread = std::thread([this] { Run(); });
}

void Discovery::Stop() {
    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void Discovery::Run() {
    std::string error;
    net::Fd fd = net::BindUdp(0, error);
    if (!fd) {
        return;  // nothing to find with
    }
    int one = 1;
    setsockopt(fd.Get(), SOL_SOCKET, SO_BROADCAST, &one, sizeof(one));

    std::vector<Pc> found;
    uint64_t nextAskMs = 0, nextSweepMs = 0;
    char buffer[2048];
    while (m_running) {
        uint64_t now = net::NowMs();
        if (now >= nextAskMs) {
            std::vector<in_addr> targets = Targets(m_emulator);
            // Not on the emulator: its network is the PC it runs on, asked above, and its
            // slow NAT would only queue the requests up.
            if (!m_emulator && now >= nextSweepMs) {
                std::vector<in_addr> neighbours = Neighbours();
                targets.insert(targets.end(), neighbours.begin(), neighbours.end());
                nextSweepMs = now + kSweepEveryMs;
            }
            for (const in_addr& target : targets) {
                sockaddr_in to = {};
                to.sin_family = AF_INET;
                to.sin_addr = target;
                to.sin_port = htons(kDiscoveryPort);
                // Without waiting: a request that doesn't fit now is dropped, and the next goes
                // out in a second. Waiting would leave the answers unread, and old by the time
                // they're read. A network that's down: the others may not be.
                sendto(fd.Get(), kDiscoveryRequest, sizeof(kDiscoveryRequest) - 1,
                       MSG_NOSIGNAL | MSG_DONTWAIT, reinterpret_cast<sockaddr*>(&to), sizeof(to));
            }
            nextAskMs = now + kAskEveryMs;
        }

        // Every answer waiting, so the latest one counts.
        bool changed = false;
        if (net::WaitReadable(fd.Get(), 250)) {
            sockaddr_in from = {};
            socklen_t fromLength = sizeof(from);
            ssize_t got;
            while ((got = recvfrom(fd.Get(), buffer, sizeof(buffer), MSG_DONTWAIT,
                                   reinterpret_cast<sockaddr*>(&from), &fromLength)) >= 0) {
                fromLength = sizeof(from);
                char host[INET_ADDRSTRLEN] = {};
                Pc pc;
                if (got == 0 || !inet_ntop(AF_INET, &from.sin_addr, host, sizeof(host)) ||
                    !ParseDiscoveryReply(std::string(buffer, size_t(got)), host, pc.reply)) {
                    continue;
                }
                pc.host = host;
                pc.lastSeenMs = net::NowMs();
                auto known = std::find_if(found.begin(), found.end(), [&pc](const Pc& other) {
                    return other.host == pc.host && other.reply.name == pc.reply.name;
                });
                if (known == found.end()) {
                    found.push_back(pc);
                    changed = true;
                } else {
                    changed |= !SamePc(*known, pc);
                    *known = pc;
                }
            }
        }
        uint64_t forgetBefore = net::NowMs() - kForgetAfterMs;
        size_t before = found.size();
        found.erase(std::remove_if(found.begin(), found.end(),
                                   [forgetBefore](const Pc& pc) { return pc.lastSeenMs < forgetBefore; }),
                    found.end());
        changed |= found.size() != before;
        if (changed && m_running) {
            m_listener(found);
        }
    }
}

}  // namespace gazeora
