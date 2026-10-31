// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <atomic>
#include <memory>
#include <mutex>

#include "head/head_pose_sender.h"
#include "head/orientation_source.h"

namespace gazeora {

// Head tracking: while the picture is shown for a VR viewer, where the viewer's head points goes
// to the PC, which turns the camera of the game in front with it. The sensor runs only then.
class HeadTracker {
public:
    HeadTracker(std::unique_ptr<OrientationSource> source, HeadPoseSender sender,
                int quarterTurns);
    ~HeadTracker();
    HeadTracker(const HeadTracker&) = delete;
    HeadTracker& operator=(const HeadTracker&) = delete;

    // Whether the picture is shown for a VR viewer. From any thread.
    void SetActive(bool active);
    // The display's rotation (Surface.ROTATION_*), which says how the phone sits in the viewer.
    void SetScreen(int quarterTurns) { m_quarterTurns = quarterTurns; }

private:
    void OnOrientation(const Quaternion& device);

    std::unique_ptr<OrientationSource> m_source;
    HeadPoseSender m_sender;  // on the source's thread
    std::atomic<int> m_quarterTurns;
    std::mutex m_activeMutex;
    bool m_active = false;  // with m_activeMutex
};

}  // namespace gazeora
