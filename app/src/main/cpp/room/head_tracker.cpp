#include "room/head_tracker.h"

#include <android/log.h>

#include <cmath>

#include "room/head_pose.h"

namespace gazeora {

namespace {

constexpr char kTag[] = "GazeoraHead";
constexpr char kPackage[] = "com.gazeora.gazeora";
constexpr int kLooperId = 3;  // any id: events are read straight off the queue
// 200 Hz: the fastest without HIGH_SAMPLING_RATE_SENSORS.
constexpr int32_t kSamplingPeriodUs = 5000;
constexpr int kEventsPerRead = 16;

}  // namespace

HeadTracker::HeadTracker() {
    m_manager = ASensorManager_getInstanceForPackage(kPackage);
    if (!m_manager) {
        return;
    }
    m_sensor = ASensorManager_getDefaultSensor(m_manager, ASENSOR_TYPE_GAME_ROTATION_VECTOR);
    if (!m_sensor) {
        m_sensor = ASensorManager_getDefaultSensor(m_manager, ASENSOR_TYPE_ROTATION_VECTOR);
    }
    ALooper* looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    if (!m_sensor || !looper) {
        __android_log_print(ANDROID_LOG_WARN, kTag,
                            "no rotation sensor: the window follows the head");
        return;
    }
    m_queue = ASensorManager_createEventQueue(m_manager, looper, kLooperId, nullptr, nullptr);
}

HeadTracker::~HeadTracker() {
    Stop();
    if (m_queue) {
        ASensorManager_destroyEventQueue(m_manager, m_queue);
    }
}

void HeadTracker::Start() {
    if (m_queue && !m_started) {
        m_started = ASensorEventQueue_registerSensor(m_queue, m_sensor, kSamplingPeriodUs, 0) == 0;
        m_known = false;
    }
}

void HeadTracker::Stop() {
    if (m_queue && m_started) {
        ASensorEventQueue_disableSensor(m_queue, m_sensor);
        m_started = false;
    }
}

bool HeadTracker::Read(int quarterTurns, Mat3& screenToWorld) {
    if (!m_started) {
        return false;
    }
    ASensorEvent events[kEventsPerRead];
    ssize_t count;
    while ((count = ASensorEventQueue_getEvents(m_queue, events, kEventsPerRead)) > 0) {
        const ASensorEvent& newest = events[count - 1];
        for (int i = 0; i < 4; ++i) m_rotation[i] = newest.data[i];
        m_known = true;
    }
    if (!m_known) {
        return false;
    }
    float x = m_rotation[0], y = m_rotation[1], z = m_rotation[2];
    float w = m_rotation[3];
    float norm = std::sqrt(x * x + y * y + z * z + w * w);
    if (norm <= 0) {
        return false;
    }
    screenToWorld = head_pose::DeviceToWorld(x / norm, y / norm, z / norm, w / norm) *
                    head_pose::ScreenToDevice(quarterTurns);
    return true;
}

}  // namespace gazeora
