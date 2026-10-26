#pragma once

#include <camera/NdkCameraCaptureSession.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraManager.h>
#include <media/NdkImageReader.h>

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>

#include "gl/hardware_buffer_textures.h"
#include "gl/image_releaser.h"
#include "room/camera_setup.h"

namespace gazeora {

// The back camera's pictures as textures, for what's behind the window. It opens on a
// thread of its own, as opening takes a moment; one that couldn't open (no permission yet,
// in use elsewhere, none) or stopped is tried again every few seconds. Everything else is on
// the render thread.
class CameraFeed {
public:
    CameraFeed(EGLDisplay display, const EglProcs& procs, const ImageReleaser& releaser);
    ~CameraFeed();
    CameraFeed(const CameraFeed&) = delete;
    CameraFeed& operator=(const CameraFeed&) = delete;

    void Open();
    void Close();
    // The newest picture's texture and crop; null until the camera sends one.
    const HardwareBufferTextures::Texture* Latest(AImageCropRect& crop);
    // Couldn't open, or stopped: the 360 scene shows instead.
    bool Failed() const { return m_state == State::Failed; }
    // The camera's; only meaningful once Latest has given a picture.
    const CameraSetup& Setup() const { return m_setup; }

private:
    enum class State { Closed, Opening, Running, Failed };

    static void OnDisconnected(void* context, ACameraDevice* device);
    static void OnError(void* context, ACameraDevice* device, int error);
    static void OnSessionState(void* context, ACameraCaptureSession* session);
    void OpenNow();
    bool Find(std::string& id);
    bool StartSession(const std::string& id);
    void Fail(const char* what, int status);
    bool RetryDue() const;
    void TearDown();

    ACameraManager* m_manager;
    HardwareBufferTextures m_textures;
    const ImageReleaser& m_releaser;
    ACameraDevice_StateCallbacks m_deviceCallbacks;
    ACameraCaptureSession_stateCallbacks m_sessionCallbacks;
    std::thread m_opener;
    std::atomic<State> m_state{State::Closed};
    std::atomic<int64_t> m_failedAtMs{0};

    // Set up by the opener; the render thread's once Running.
    CameraSetup m_setup;
    ACameraDevice* m_device = nullptr;
    AImageReader* m_reader = nullptr;
    ACaptureSessionOutputContainer* m_outputs = nullptr;
    ACaptureSessionOutput* m_output = nullptr;
    ACameraOutputTarget* m_target = nullptr;
    ACaptureRequest* m_request = nullptr;
    ACameraCaptureSession* m_session = nullptr;

    // The picture last drawn, kept until a newer one comes: it's drawn again every frame.
    AImage* m_held = nullptr;
    AHardwareBuffer* m_heldBuffer = nullptr;
    AImageCropRect m_heldCrop = {};
};

}  // namespace gazeora
