// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "hands/hand_tracker.h"

#include <android/log.h>

#include <algorithm>
#include <chrono>
#include <cstring>


namespace gazeora {

namespace {

constexpr char kTag[] = "GazeoraHands";
// A dim picture is brightened toward this average (0 to 1), by at most this much.
constexpr float kBrightEnough = 0.45f;
constexpr float kMostGain = 4.0f;
// How often to say how fast it is, in seconds.
constexpr double kReportEvery = 2.0;

double Seconds(std::chrono::steady_clock::time_point from) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - from).count();
}

float Gain(float meanLuma) {
    return meanLuma > 0 ? std::clamp(kBrightEnough / meanLuma, 1.0f, kMostGain) : 1.0f;
}

}  // namespace

HandTracker::HandTracker(HandModelFiles files) : m_files(std::move(files)) {}

HandTracker::~HandTracker() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_quitting = true;
    }
    m_wake.notify_all();
    if (m_thread.joinable()) {
        m_thread.join();
    }
    HandModels::Give(std::move(m_models));  // for the next player
}

void HandTracker::Start() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_running = true;
    if (!m_thread.joinable()) {
        m_thread = std::thread([this] { Run(); });
    }
}

void HandTracker::Stop() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_running = false;
    m_waiting = false;
    m_forget = true;
    m_hands.clear();
    m_handsNew = false;
}

void HandTracker::Track(const YuvPlanes& yuv, const CameraView& view, int64_t timestampNs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_running || !m_usable) {
        return;
    }
    Copy(yuv, m_next);
    m_next.view = view;
    m_next.timestampNs = timestampNs;
    m_waiting = true;
    m_wake.notify_all();
}

bool HandTracker::Take(std::vector<HandLandmarks>& hands) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_handsNew) {
        return false;
    }
    hands = m_hands;
    m_handsNew = false;
    return true;
}

void HandTracker::Copy(const YuvPlanes& yuv, Picture& out) {
    // Packed luma rows; the chroma as it is, from its first sample to its last.
    out.y.resize(size_t(yuv.width) * yuv.height);
    for (int32_t row = 0; row < yuv.height; ++row) {
        std::memcpy(out.y.data() + size_t(row) * yuv.width, yuv.y + size_t(row) * yuv.yRowStride,
                    size_t(yuv.width));
    }
    const size_t chroma = size_t((yuv.height + 1) / 2 - 1) * yuv.uvRowStride +
                          size_t((yuv.width + 1) / 2 - 1) * yuv.uvPixelStride + 1;
    out.u.assign(yuv.u, yuv.u + chroma);
    out.v.assign(yuv.v, yuv.v + chroma);
    out.yuv = yuv;
    out.yuv.y = out.y.data();
    out.yuv.u = out.u.data();
    out.yuv.v = out.v.data();
    out.yuv.yRowStride = yuv.width;
}

void HandTracker::Run() {
    std::string error;
    m_models = HandModels::Take(m_files, error);
    if (!m_models) {
        __android_log_print(ANDROID_LOG_WARN, kTag, "no hand tracking: %s", error.c_str());
        std::lock_guard<std::mutex> lock(m_mutex);
        m_usable = false;
        return;
    }
    __android_log_print(ANDROID_LOG_INFO, kTag, "hand tracking: %s", m_models->Where());
    while (true) {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_wake.wait(lock, [this] { return m_quitting || m_waiting; });
            if (m_quitting) {
                return;
            }
            std::swap(m_next, m_looking);
            m_waiting = false;
            if (m_forget) {
                m_tracking.Clear();
                m_forget = false;
            }
        }
        HandModels::Faster(m_models);
        Look(m_looking);
    }
}

void HandTracker::Look(const Picture& picture) {
    using namespace hand_detection;
    using camera_frame::CropRgb;
    using camera_frame::MeanLuma;
    const auto started = std::chrono::steady_clock::now();
    const YuvPlanes& yuv = picture.yuv;
    const int(&toSensor)[2][2] = picture.view.toSensor;
    const bool quarter = toSensor[0][0] == 0;
    const int width = quarter ? yuv.height : yuv.width;
    const int height = quarter ? yuv.width : yuv.height;
    const double now = picture.timestampNs / 1e9;

    if (m_tracking.Searching(now)) {
        const auto searching = std::chrono::steady_clock::now();
        const PictureSquare whole = WholePicture(width, height);
        CropRgb(yuv, toSensor, whole, kPalmSide, Gain(MeanLuma(yuv, toSensor, whole)),
                m_models->PalmInput());
        if (m_models->FindPalms()) {
            m_tracking.Search(ReadPalms(m_models->Boxes(), m_models->Scores(), whole), now);
        }
        m_stats.searchSeconds += Seconds(searching);
        ++m_stats.searches;
    }
    const std::vector<PictureSquare>& squares = m_tracking.Squares();
    for (size_t i = 0; i < squares.size(); ++i) {
        const auto looking = std::chrono::steady_clock::now();
        CropRgb(yuv, toSensor, squares[i], kHandSide, Gain(MeanLuma(yuv, toSensor, squares[i])),
                m_models->HandInput());
        if (m_models->FindHand()) {
            m_tracking.Answer(i, m_models->Presence(),
                              ReadHand(squares[i], m_models->Landmarks(), m_models->Shape(),
                                       m_models->Handedness(), width, height),
                              width, height);
        }
        m_stats.handSeconds += Seconds(looking);
        ++m_stats.hands;
    }
    std::vector<HandLandmarks> hands = m_tracking.Finish();
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_forget) {
            m_hands = std::move(hands);
            m_handsNew = true;
        }
    }

    ++m_stats.pictures;
    m_stats.busySeconds += Seconds(started);
    const double since = Seconds(m_stats.since);
    if (since >= kReportEvery) {
        __android_log_print(
            ANDROID_LOG_DEBUG, kTag,
            "looked at %.1f pictures a second, busy %.0f%%; finding hands %.0f ms (%d), "
            "following one %.0f ms (%d); %s",
            m_stats.pictures / since, 100 * m_stats.busySeconds / since,
            m_stats.searches ? 1000 * m_stats.searchSeconds / m_stats.searches : 0.0,
            m_stats.searches, m_stats.hands ? 1000 * m_stats.handSeconds / m_stats.hands : 0.0,
            m_stats.hands, m_models->Where());
        m_stats = Stats();
    }
}

}  // namespace gazeora
