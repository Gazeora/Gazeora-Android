// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <android/sensor.h>

#include <atomic>
#include <thread>

#include "head/orientation_source.h"

namespace gazeora {

// Adapter pattern: Android's game rotation vector sensor (gyroscope and accelerometer, no
// compass to pull it around near metal) as an OrientationSource, on a thread of its own.
// Phones without one fall back to the plain rotation vector.
class RotationSensor final : public OrientationSource {
public:
    RotationSensor() = default;
    ~RotationSensor() override { Stop(); }
    RotationSensor(const RotationSensor&) = delete;
    RotationSensor& operator=(const RotationSensor&) = delete;

    void Start(Listener listener) override;
    void Stop() override;

private:
    void Run();
    // Every event waiting, and the newest of them to the listener: older ones are stale.
    void Drain(ASensorEventQueue* queue);

    Listener m_listener;
    std::atomic<bool> m_stopping{false};
    std::thread m_thread;
};

}  // namespace gazeora
