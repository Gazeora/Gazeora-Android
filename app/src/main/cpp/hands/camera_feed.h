// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <camera/NdkCameraCaptureSession.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraManager.h>
#include <media/NdkImageReader.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include "hands/camera_frame.h"
#include "hands/camera_setup.h"
#include "hands/light_estimate.h"

namespace gazeora {

// The back camera, for hand tracking only: its small pictures go to the hand tracker as they
// come, and now and then give the room's light to light the drawn hands by; none is shown. It
// opens on a thread of its own, as opening takes a moment; one that couldn't open (no
// permission yet, in use elsewhere, none) or stopped is tried again every few seconds.
// Everything else is on the render thread, but the pictures.
class CameraFeed {
public:
    // Each picture for hand tracking, from the camera's own thread, with what stands it upright
    // (CameraView::toSensor); the planes last the call.
    using FrameSink =
        std::function<void(const YuvPlanes& yuv, const CameraView& view, int64_t timestampNs)>;

    explicit CameraFeed(FrameSink sink);
    ~CameraFeed();
    CameraFeed(const CameraFeed&) = delete;
    CameraFeed& operator=(const CameraFeed&) = delete;

    void Open();
    void Close();
    // Every frame while open: tries again once it's time.
    void Poll();
    bool Running() const { return m_state == State::Running; }
    // The display's turn (Surface.ROTATION_*), which stands the copies upright.
    void SetQuarterTurns(int quarterTurns) { m_quarterTurns = quarterTurns; }
    // The camera's; only meaningful while Running.
    const CameraSetup& Setup() const { return m_setup; }
    // The room's light, as the latest picture shows it; from any thread.
    SceneLight Light();

private:
    enum class State { Closed, Opening, Running, Failed };

    static void OnDisconnected(void* context, ACameraDevice* device);
    static void OnError(void* context, ACameraDevice* device, int error);
    static void OnSessionState(void* context, ACameraCaptureSession* session);
    static void OnImage(void* context, AImageReader* reader);
    void OpenNow();
    bool Find(std::string& id);
    bool StartSession(const std::string& id);
    void Fail(const char* what, int status);
    bool RetryDue() const;
    void TearDown();

    ACameraManager* m_manager;
    ACameraDevice_StateCallbacks m_deviceCallbacks;
    ACameraCaptureSession_stateCallbacks m_sessionCallbacks;
    std::thread m_opener;
    std::atomic<State> m_state{State::Closed};
    std::atomic<int64_t> m_failedAtMs{0};
    FrameSink m_sink;
    std::atomic<int> m_quarterTurns{1};

    // Set up by the opener; the render thread's once Running.
    CameraSetup m_setup;
    ACameraDevice* m_device = nullptr;
    AImageReader* m_reader = nullptr;
    ACaptureSessionOutputContainer* m_outputs = nullptr;
    ACaptureSessionOutput* m_output = nullptr;
    ACameraOutputTarget* m_target = nullptr;
    ACaptureRequest* m_request = nullptr;
    ACameraCaptureSession* m_session = nullptr;
    std::mutex m_readerMutex;
    bool m_readerOpen = false;  // with m_readerMutex: the reader's callback may use it
    RgbaFrame m_frame;          // the callback's: for the light
    int m_sinceLight = 0;       // the callback's: pictures since it last looked at the light
    std::mutex m_lightMutex;
    SceneLight m_light;  // with m_lightMutex
};

}  // namespace gazeora
