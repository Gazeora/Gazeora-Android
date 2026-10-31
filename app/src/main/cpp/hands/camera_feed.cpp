// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "hands/camera_feed.h"

#include <android/log.h>

#include <chrono>

#include "hands/view_geometry.h"

namespace gazeora {

namespace {

constexpr char kTag[] = "GazeoraCamera";
// One being copied, one arriving.
constexpr int32_t kReaderImages = 2;
constexpr int64_t kRetryAfterMs = 3000;
// The room's light changes slowly: it's looked at in one picture of this many.
constexpr int kLightEvery = 6;

int64_t NowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

}  // namespace

CameraFeed::CameraFeed(FrameSink sink)
    : m_manager(ACameraManager_create()),
      m_deviceCallbacks(),
      m_sessionCallbacks(),
      m_sink(std::move(sink)) {
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

void CameraFeed::Poll() {
    if (RetryDue()) {
        Close();
        Open();
    }
}

void CameraFeed::OnDisconnected(void* context, ACameraDevice*) {
    static_cast<CameraFeed*>(context)->Fail("camera taken away", 0);
}

void CameraFeed::OnError(void* context, ACameraDevice*, int error) {
    static_cast<CameraFeed*>(context)->Fail("camera error", error);
}

void CameraFeed::OnSessionState(void*, ACameraCaptureSession*) {}

void CameraFeed::OnImage(void* context, AImageReader* reader) {
    auto* self = static_cast<CameraFeed*>(context);
    // Not once it's being torn down: the reader is about to go.
    std::lock_guard<std::mutex> lock(self->m_readerMutex);
    if (!self->m_readerOpen) {
        return;
    }
    AImage* image = nullptr;
    if (AImageReader_acquireLatestImage(reader, &image) != AMEDIA_OK || !image) {
        return;
    }
    YuvPlanes yuv;
    int yLength = 0, uLength = 0, vLength = 0;
    int64_t timestampNs = 0;
    uint8_t *y = nullptr, *u = nullptr, *v = nullptr;
    bool usable = AImage_getWidth(image, &yuv.width) == AMEDIA_OK &&
                AImage_getHeight(image, &yuv.height) == AMEDIA_OK &&
                AImage_getPlaneData(image, 0, &y, &yLength) == AMEDIA_OK &&
                AImage_getPlaneData(image, 1, &u, &uLength) == AMEDIA_OK &&
                AImage_getPlaneData(image, 2, &v, &vLength) == AMEDIA_OK &&
                AImage_getPlaneRowStride(image, 0, &yuv.yRowStride) == AMEDIA_OK &&
                AImage_getPlaneRowStride(image, 1, &yuv.uvRowStride) == AMEDIA_OK &&
                AImage_getPlanePixelStride(image, 1, &yuv.uvPixelStride) == AMEDIA_OK &&
                AImage_getTimestamp(image, &timestampNs) == AMEDIA_OK && y && u && v &&
                yuv.width > 0 && yuv.height > 0;
    if (usable) {
        yuv.y = y;
        yuv.u = u;
        yuv.v = v;
        CameraView view = self->m_setup.view;
        camera_geometry::SetOrientation(view, self->m_setup.sensorOrientation,
                                        self->m_quarterTurns);
        self->m_sink(yuv, view, timestampNs);
        if (++self->m_sinceLight >= kLightEvery) {
            self->m_sinceLight = 0;
            camera_frame::UprightRgba(yuv, view.toSensor, self->m_frame);
            float across = 0, down = 0;
            camera_geometry::UprightTangents(view, across, down);
            SceneLight light = light_estimate::Estimate(self->m_frame, across, down);
            std::lock_guard<std::mutex> lightLock(self->m_lightMutex);
            self->m_light = light;
        }
    }
    AImage_delete(image);
}

SceneLight CameraFeed::Light() {
    std::lock_guard<std::mutex> lock(m_lightMutex);
    return m_light;
}

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
    media_status_t media =
        AImageReader_new(m_setup.size.width, m_setup.size.height, AIMAGE_FORMAT_YUV_420_888,
                         kReaderImages, &m_reader);
    if (media != AMEDIA_OK || AImageReader_getWindow(m_reader, &window) != AMEDIA_OK) {
        Fail("no image reader for the camera", media);
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(m_readerMutex);
        m_readerOpen = true;
    }
    AImageReader_ImageListener listener = {this, &CameraFeed::OnImage};
    AImageReader_setImageListener(m_reader, &listener);
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
    if (m_setup.frameRates.most > 0) {
        const int32_t rates[2] = {m_setup.frameRates.least, m_setup.frameRates.most};
        ACaptureRequest_setEntry_i32(m_request, ACAMERA_CONTROL_AE_TARGET_FPS_RANGE, 2, rates);
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
    __android_log_print(ANDROID_LOG_INFO, kTag,
                        "camera %s tracking at %dx%d, %d-%d frames a second, zoom %.2f",
                        id.c_str(), m_setup.size.width, m_setup.size.height,
                        m_setup.frameRates.least, m_setup.frameRates.most, m_setup.zoomRatio);
    return true;
}

void CameraFeed::Fail(const char* what, int status) {
    __android_log_print(ANDROID_LOG_WARN, kTag, "%s (status %d): no hands for now", what,
                        status);
    m_failedAtMs = NowMs();
    m_state = State::Failed;
}

bool CameraFeed::RetryDue() const {
    return m_state == State::Failed && NowMs() - m_failedAtMs >= kRetryAfterMs;
}

void CameraFeed::TearDown() {
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
    if (m_reader) {
        {
            // A copy being made finishes first; after that none touches the reader, which
            // waits for its callbacks as it goes.
            std::lock_guard<std::mutex> lock(m_readerMutex);
            m_readerOpen = false;
        }
        AImageReader_setImageListener(m_reader, nullptr);
        AImageReader_delete(m_reader);
    }
    m_request = nullptr;
    m_target = nullptr;
    m_output = nullptr;
    m_outputs = nullptr;
    m_reader = nullptr;
}

}  // namespace gazeora
