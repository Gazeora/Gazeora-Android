#pragma once

#include <android/looper.h>
#include <android/sensor.h>

#include "room/mat3.h"

namespace gazeora {

// Which way the phone, and so the head, points: the game rotation vector (gyroscope and
// accelerometer, no compass to jump near metal), or the rotation vector where there's none.
// Made, used and destroyed on one thread: the render thread, which reads it every frame.
class HeadTracker {
public:
    HeadTracker();
    ~HeadTracker();
    HeadTracker(const HeadTracker&) = delete;
    HeadTracker& operator=(const HeadTracker&) = delete;

    // Readings come only while started, to spare the battery.
    void Start();
    void Stop();
    // The newest screen-to-world rotation for a display turned `quarterTurns`; false before
    // the first reading, or without a sensor.
    bool Read(int quarterTurns, Mat3& screenToWorld);

private:
    ASensorManager* m_manager = nullptr;
    const ASensor* m_sensor = nullptr;
    ASensorEventQueue* m_queue = nullptr;
    bool m_started = false;
    bool m_known = false;
    float m_rotation[4] = {0, 0, 0, 1};  // x, y, z, w
};

}  // namespace gazeora
