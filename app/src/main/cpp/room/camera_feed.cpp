#include "room/camera_feed.h"

#include <android/log.h>

#include <chrono>

namespace gazeora {

namespace {

constexpr char kTag[] = "GazeoraCamera";
// One being drawn, one arriving, one spare.
constexpr int32_t kReaderImages = 3;
constexpr size_t kMaxTextures = 6;
constexpr int64_t kRetryAfterMs = 3000;

int64_t NowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

}  // namespace

CameraFeed::CameraFeed(EGLDisplay display, const EglProcs& procs, const ImageReleaser& releaser)
    : m_manager(ACameraManager_create()),
      m_textures(display, procs, kMaxTextures),
      m_releaser(releaser),
      m_deviceCallbacks(),
      m_sessionCallbacks() {
    // By name: newer NDKs add fields, which stay null.
    m_deviceCallbacks.context = this;
    m_deviceCallbacks.onDisconnected = &CameraFeed::OnDisconnected;
    m_deviceCallbacks.onError = &CameraFeed::OnError;
    m_sessionCallbacks.context = this;
    m_sessionCallbacks.onClosed = &CameraFeed::OnSessionState;
    m_sessionCallbacks.onReady = &CameraFeed::OnSessionState;
    m_sessionCallbacks.onActive = &CameraFeed::OnSessionState;
}

CameraFeed::~CameraFeed() {
    Close();
    ACameraManager_delete(m_manager);
}

void CameraFeed::Open() {
    if (m_state != State::Closed) {
        return;
    }
    m_state = State::Opening;
    m_opener = std::thread([this] { OpenNow(); });
}

void CameraFeed::Close() {
    if (m_opener.joinable()) {
        m_opener.join();
    }
    TearDown();
    m_state = State::Closed;
}

const HardwareBufferTextures::Texture* CameraFeed::Latest(AImageCropRect& crop) {
    if (RetryDue()) {
        Close();
        Open();
    }
    if (m_state != State::Running) {
        return nullptr;
    }
    AImage* image = nullptr;
    if (AImageReader_acquireLatestImage(m_reader, &image) == AMEDIA_OK && image) {
        AHardwareBuffer* buffer = nullptr;
        AImage_getHardwareBuffer(image, &buffer);
        if (buffer && m_textures.Make(buffer)) {
            m_releaser.Release(m_held);  // after the frames that drew it
            m_held = image;
            m_heldBuffer = buffer;
            AImage_getCropRect(image, &m_heldCrop);
        } else {
            AImage_delete(image);
        }
    }
    crop = m_heldCrop;
    return m_held ? m_textures.Find(m_heldBuffer) : nullptr;
}

void CameraFeed::OnDisconnected(void* context, ACameraDevice*) {
    static_cast<CameraFeed*>(context)->Fail("camera taken away", 0);
}

void CameraFeed::OnError(void* context, ACameraDevice*, int error) {
    static_cast<CameraFeed*>(context)->Fail("camera error", error);
}

void CameraFeed::OnSessionState(void*, ACameraCaptureSession*) {}

void CameraFeed::OpenNow() {
    std::string id;
    if (!Find(id)) {
        Fail("no back camera to use", 0);
        return;
    }
    if (!StartSession(id)) {
        return;
    }
    State opening = State::Opening;  // unless it failed meanwhile
    m_state.compare_exchange_strong(opening, State::Running);
}

bool CameraFeed::Find(std::string& id) {
    ACameraIdList* ids = nullptr;
    if (ACameraManager_getCameraIdList(m_manager, &ids) != ACAMERA_OK || !ids) {
        return false;
    }
    bool found = false;
    // The first back camera is the main one.
    for (int i = 0; i < ids->numCameras && !found; ++i) {
        ACameraMetadata* characteristics = nullptr;
        if (ACameraManager_getCameraCharacteristics(m_manager, ids->cameraIds[i],
                                                    &characteristics) != ACAMERA_OK) {
            continue;
        }
        found = m_setup.Read(characteristics);
        if (found) {
            id = ids->cameraIds[i];
        }
        ACameraMetadata_free(characteristics);
    }
    ACameraManager_deleteCameraIdList(ids);
    return found;
}

bool CameraFeed::StartSession(const std::string& id) {
    camera_status_t status =
        ACameraManager_openCamera(m_manager, id.c_str(), &m_deviceCallbacks, &m_device);
    if (status != ACAMERA_OK) {
        Fail("can't open the camera", status);  // no permission (yet), or it's in use
        return false;
    }
    ANativeWindow* window = nullptr;
    media_status_t media = AImageReader_newWithUsage(
        m_setup.size.width, m_setup.size.height, AIMAGE_FORMAT_PRIVATE,
        AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE, kReaderImages, &m_reader);
    if (media != AMEDIA_OK || AImageReader_getWindow(m_reader, &window) != AMEDIA_OK) {
        Fail("no image reader for the camera", media);
        return false;
    }
    if (ACaptureSessionOutputContainer_create(&m_outputs) != ACAMERA_OK ||
        ACaptureSessionOutput_create(window, &m_output) != ACAMERA_OK ||
        ACaptureSessionOutputContainer_add(m_outputs, m_output) != ACAMERA_OK ||
        ACameraOutputTarget_create(window, &m_target) != ACAMERA_OK ||
        ACameraDevice_createCaptureRequest(m_device, TEMPLATE_PREVIEW, &m_request) != ACAMERA_OK ||
        ACaptureRequest_addTarget(m_request, m_target) != ACAMERA_OK) {
        Fail("can't set up the camera's stream", 0);
        return false;
    }
    if (m_setup.zoomRatio < 1.0f) {
        ACaptureRequest_setEntry_float(m_request, ACAMERA_CONTROL_ZOOM_RATIO, 1,
                                       &m_setup.zoomRatio);
    }
    status =
        ACameraDevice_createCaptureSession(m_device, m_outputs, &m_sessionCallbacks, &m_session);
    if (status == ACAMERA_OK) {
        status =
            ACameraCaptureSession_setRepeatingRequest(m_session, nullptr, 1, &m_request, nullptr);
    }
    if (status != ACAMERA_OK) {
        Fail("can't start the camera", status);
        return false;
    }
    __android_log_print(ANDROID_LOG_INFO, kTag, "camera %s streaming %dx%d at zoom %.2f",
                        id.c_str(), m_setup.size.width, m_setup.size.height, m_setup.zoomRatio);
    return true;
}

void CameraFeed::Fail(const char* what, int status) {
    __android_log_print(ANDROID_LOG_WARN, kTag, "%s (status %d): showing the 360 scene", what,
                        status);
    m_failedAtMs = NowMs();
    m_state = State::Failed;
}

bool CameraFeed::RetryDue() const {
    return m_state == State::Failed && NowMs() - m_failedAtMs >= kRetryAfterMs;
}

void CameraFeed::TearDown() {
    // The held picture goes before its reader, which takes all its pictures with it.
    if (m_held) {
        glFinish();
        AImage_delete(m_held);
        m_held = nullptr;
        m_heldBuffer = nullptr;
    }
    m_textures.Clear();
    if (m_session) {
        ACameraCaptureSession_stopRepeating(m_session);
        ACameraCaptureSession_close(m_session);
        m_session = nullptr;
    }
    if (m_device) {
        ACameraDevice_close(m_device);
        m_device = nullptr;
    }
    if (m_request) ACaptureRequest_free(m_request);
    if (m_target) ACameraOutputTarget_free(m_target);
    if (m_output) ACaptureSessionOutput_free(m_output);
    if (m_outputs) ACaptureSessionOutputContainer_free(m_outputs);
    if (m_reader) AImageReader_delete(m_reader);
    m_request = nullptr;
    m_target = nullptr;
    m_output = nullptr;
    m_outputs = nullptr;
    m_reader = nullptr;
}

}  // namespace gazeora
