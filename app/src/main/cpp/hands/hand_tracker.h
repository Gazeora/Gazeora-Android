#pragma once

#include <android/asset_manager.h>

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "hands/camera_frame.h"
#include "hands/hand_geometry.h"
#include "hands/hand_models.h"
#include "hands/hand_tracking.h"
#include "hands/view_geometry.h"

namespace gazeora {

// Finds the viewer's hands in the back camera's pictures with MediaPipe's hand landmarker
// models, run with LiteRT each on this phone's GPU or CPU, whichever runs it faster (timed as
// it starts), on a thread of its own. Each
// picture is cut straight from the camera's YUV into what the models look at: the whole of it
// only to find hands, and once found, a square around each hand (HandTracking). Dim pictures
// are brightened first, so it still finds hands in a dim room. A picture that comes while
// it's busy waits for it, in place of any older one.
class HandTracker {
public:
    explicit HandTracker(HandModelFiles files);
    ~HandTracker();
    HandTracker(const HandTracker&) = delete;
    HandTracker& operator=(const HandTracker&) = delete;

    // Starts looking at the pictures given it; Stop stops, and forgets the hands.
    void Start();
    void Stop();
    // A camera picture, from the camera's thread; `view` stands it upright. Copied before it
    // returns.
    void Track(const YuvPlanes& yuv, const CameraView& view, int64_t timestampNs);
    // The hands in the newest picture looked at, into `hands`, if it's newer than the last
    // taken; from any thread.
    bool Take(std::vector<HandLandmarks>& hands);

private:
    struct Picture {
        std::vector<uint8_t> y, u, v;
        YuvPlanes yuv;
        CameraView view;
        int64_t timestampNs = 0;
    };
    // How fast it's been, since `since`.
    struct Stats {
        std::chrono::steady_clock::time_point since = std::chrono::steady_clock::now();
        int pictures = 0;
        int searches = 0;
        int hands = 0;
        double busySeconds = 0;
        double searchSeconds = 0;
        double handSeconds = 0;
    };

    void Run();
    void Look(const Picture& picture);
    static void Copy(const YuvPlanes& yuv, Picture& out);

    HandModelFiles m_files;
    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_wake;
    bool m_running = false;   // with m_mutex
    bool m_quitting = false;  // with m_mutex
    bool m_usable = true;     // with m_mutex: false once the models can't run
    bool m_waiting = false;   // with m_mutex: m_next holds a picture
    bool m_forget = false;    // with m_mutex: the hands followed are gone
    Picture m_next;           // with m_mutex
    std::vector<HandLandmarks> m_hands;  // with m_mutex
    bool m_handsNew = false;  // with m_mutex

    // The thread's.
    std::unique_ptr<HandModels> m_models;
    HandTracking m_tracking;
    Picture m_looking;
    Stats m_stats;
};

}  // namespace gazeora
