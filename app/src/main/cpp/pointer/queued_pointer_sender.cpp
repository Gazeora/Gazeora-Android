// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "pointer/queued_pointer_sender.h"

namespace gazeora {

namespace {

// Touches waiting beyond this mean the PC isn't taking them; newer ones are dropped.
constexpr size_t kMaxQueued = 256;

}  // namespace

QueuedPointerSender::QueuedPointerSender(std::unique_ptr<PointerSender> inner)
    : m_inner(std::move(inner)), m_thread([this] { Run(); }) {}

QueuedPointerSender::~QueuedPointerSender() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stopping = true;
    }
    m_wake.notify_all();
    m_thread.join();
}

void QueuedPointerSender::Send(const PointerEvent& event) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.size() >= kMaxQueued) {
            return;
        }
        m_queue.push_back(event);
    }
    m_wake.notify_all();
}

void QueuedPointerSender::Run() {
    std::unique_lock<std::mutex> lock(m_mutex);
    while (true) {
        m_wake.wait(lock, [this] { return m_stopping || !m_queue.empty(); });
        // What's queued still goes: a button let go as playing stops mustn't stay held.
        if (m_queue.empty()) {
            return;
        }
        PointerEvent event = m_queue.front();
        m_queue.pop_front();
        lock.unlock();
        m_inner->Send(event);
        lock.lock();
    }
}

}  // namespace gazeora
