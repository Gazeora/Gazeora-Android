// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>
#include <vector>

#include "protocols/discovery_reply.h"

namespace gazeora {

// Finds PCs running Gazeora, so nobody has to type an address. Once a second it asks the
// network (broadcast) and, on the emulator, the PC it runs on (10.0.2.2); every few seconds it
// asks each address on the network too, as many Wi-Fi networks drop broadcasts. Every Gazeora
// that hears it answers with its stream (Gazeora: discovery.h). PCs that stop answering drop
// off.
class Discovery {
public:
    struct Pc {
        std::string host;
        DiscoveryReply reply;
        uint64_t lastSeenMs = 0;
    };
    // From its thread, whenever the list changes.
    using Listener = std::function<void(const std::vector<Pc>&)>;

    Discovery(bool emulator, Listener listener)
        : m_emulator(emulator), m_listener(std::move(listener)) {}
    ~Discovery() { Stop(); }

    void Start();
    // Waits for its thread; no calls to the listener after.
    void Stop();

private:
    void Run();

    bool m_emulator;
    Listener m_listener;
    std::atomic<bool> m_running{false};
    std::thread m_thread;
};

}  // namespace gazeora
