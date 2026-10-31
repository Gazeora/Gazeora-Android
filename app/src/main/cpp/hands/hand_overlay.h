#pragma once

#include <vector>

#include "hands/hand_follower.h"
#include "hands/hand_geometry.h"
#include "hands/hand_pointer.h"
#include "hands/view_geometry.h"

namespace gazeora {

// The viewer's hands as the hand tracker last saw them, to draw over the window: the
// tracker's answers go through a HandFollower, which keeps only the viewer's own. Used on the
// render thread.
class HandOverlay {
public:
    // The tracker's newest: every hand it saw, the viewer's or not.
    void Set(std::vector<HandLandmarks> hands);
    void Clear();
    // The skeletons of the viewer's own hands to draw now, nearest first, where the camera
    // seeing as `camera` says puts them.
    std::vector<HandSkeleton> Hands(const CameraView& camera);
    // What the viewer's hand pointing at the window has the PC do since the last call: the
    // right hand points, or the left without it (HandPointer).
    std::vector<PointerEvent> TakePointerEvents();
    // Where to draw its cursor now.
    HandPointer::Cursor Cursor();

private:
    std::vector<HandLandmarks> m_seen;
    bool m_seenNew = false;  // m_seen hasn't gone to m_follower yet
    double m_seenAt = 0;
    double m_loggedAt = 0;
    int m_answers = 0;  // since m_loggedAt
    HandFollower m_follower;
    HandPointer m_pointer;
    // Where the hand pointing was last, to keep to it.
    ViewTangent m_pointingAt;
    bool m_pointing = false;
    std::vector<PointerEvent> m_pointerEvents;
};

}  // namespace gazeora
