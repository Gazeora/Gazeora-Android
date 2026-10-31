// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "head/rotation_sensor.h"

#include <android/log.h>
#include <android/looper.h>

namespace gazeora {

namespace {

constexpr char kTag[] = "GazeoraHead";
constexpr char kPackage[] = "com.gazeora.gazeora";
// 200 Hz: as fast as apps may ask without the high sampling rate permission.
constexpr int32_t kSamplingUs = 5000;
// How long Stop may wait for the thread to notice.
constexpr int kPollMs = 50;
constexpr int kEventsId = 1;
constexpr size_t kEventBatch = 16;

const ASensor* FindSensor(ASensorManager* manager) {
    const ASensor* sensor =
        ASensorManager_getDefaultSensor(manager, ASENSOR_TYPE_GAME_ROTATION_VECTOR);
    return sensor ? sensor : ASensorManager_getDefaultSensor(manager, ASENSOR_TYPE_ROTATION_VECTOR);
}

}  // namespace

void RotationSensor::Start(Listener listener) {
    if (m_thread.joinable()) {
        return;
    }
    m_listener = std::move(listener);
    m_stopping = false;
    m_thread = std::thread([this] { Run(); });
}

void RotationSensor::Stop() {
    m_stopping = true;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void RotationSensor::Run() {
    // Polled by its id, without a callback, which a looper refuses unless asked to allow it.
    ALooper* looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    ASensorManager* manager = ASensorManager_getInstanceForPackage(kPackage);
    const ASensor* sensor = manager ? FindSensor(manager) : nullptr;
    if (!sensor) {
        __android_log_print(ANDROID_LOG_WARN, kTag, "no rotation sensor: no head tracking");
        return;
    }
    ASensorEventQueue* queue =
        ASensorManager_createEventQueue(manager, looper, kEventsId, nullptr, nullptr);
    if (!queue || ASensorEventQueue_registerSensor(queue, sensor, kSamplingUs, 0) != 0) {
        __android_log_print(ANDROID_LOG_WARN, kTag, "rotation sensor won't start");
    } else {
        while (!m_stopping) {
            if (ALooper_pollOnce(kPollMs, nullptr, nullptr, nullptr) == kEventsId) {
                Drain(queue);
            }
        }
        ASensorEventQueue_disableSensor(queue, sensor);
    }
    if (queue) {
        ASensorManager_destroyEventQueue(manager, queue);
    }
}

void RotationSensor::Drain(ASensorEventQueue* queue) {
    ASensorEvent events[kEventBatch];
    ASensorEvent newest;
    bool any = false;
    ssize_t got;
    while ((got = ASensorEventQueue_getEvents(queue, events, kEventBatch)) > 0) {
        newest = events[got - 1];
        any = true;
    }
    if (any) {
        // The rotation vector's x, y, z, then w (cos of half the angle).
        m_listener(Quaternion{newest.data[0], newest.data[1], newest.data[2], newest.data[3]});
    }
}

}  // namespace gazeora
