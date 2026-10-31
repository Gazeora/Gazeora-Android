// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>

#include "pointer/pointer_sender.h"

namespace gazeora {

// Decorator pattern: sends through another sender on a thread of its own, so the touches
// arriving on the UI thread never wait for the network or the cable.
class QueuedPointerSender final : public PointerSender {
public:
    explicit QueuedPointerSender(std::unique_ptr<PointerSender> inner);
    ~QueuedPointerSender() override;
    QueuedPointerSender(const QueuedPointerSender&) = delete;
    QueuedPointerSender& operator=(const QueuedPointerSender&) = delete;

    void Send(const PointerEvent& event) override;

private:
    void Run();

    std::unique_ptr<PointerSender> m_inner;
    std::mutex m_mutex;
    std::condition_variable m_wake;
    std::deque<PointerEvent> m_queue;
    bool m_stopping = false;
    std::thread m_thread;  // last: it starts once the rest is set up
};

}  // namespace gazeora
