#pragma once

#include <cstdint>
#include <vector>

#include "hands/hand_skeleton.h"

namespace gazeora {

// A corner of the viewer's hand, posed, as hand.vert takes it: in meters, in the camera's space,
// with its surface's outward normal, how much of it shows (it fades out toward the wrist, whose
// turn the tracker doesn't see) and how much it glows (its finger pinching).
struct HandVertex {
    float x = 0;
    float y = 0;
    float z = 0;
    float nx = 0;
    float ny = 0;
    float nz = 0;
    float shows = 0;
    float glow = 0;
};

// The viewer's hand as a VR headset shows it: a hand model's skin (hand_model_data.h) rigged to
// the skeleton, each corner going with the bones it's bound to and blending between them where
// they meet, so it bends at the joints as a hand does.
class RiggedHand {
public:
    // The palm, then each finger's bones from the hand out, the thumb first.
    static constexpr int kBones = 1 + HandSkeleton::kFingers * HandSkeleton::kBones;
    // The most bones a corner goes with.
    static constexpr int kInfluences = 4;

    // The one hand, made the first time it's asked for.
    static const RiggedHand& Get();

    // Its triangles, as indices into Pose's corners.
    const std::vector<uint16_t>& Triangles() const { return m_triangles; }
    size_t Corners() const { return m_corners.size(); }
    // The skeleton the skin is bound to: the model's rest pose, a right hand at the origin,
    // along x with the thumb's side toward y.
    const HandSkeleton& Rest() const { return m_rest; }

    // Its corners on `skeleton` (mirrored, for a left hand), in `corners`; each bone glowing
    // as much as `glow` says (by RiggedHand's numbering), or none without it.
    void Pose(const HandSkeleton& skeleton, std::vector<HandVertex>& corners,
              const float* glow = nullptr) const;

private:
    struct Influence {
        int bone = 0;
        float weight = 0;
        Vec3 at;      // the corner in the bone's axes at rest: along, palmward, across
        Vec3 normal;  // its normal, the same way
    };
    struct Corner {
        Influence influences[kInfluences];
        int count = 0;
        float shows = 0;
    };

    RiggedHand();

    HandSkeleton m_rest;
    std::vector<Corner> m_corners;
    std::vector<uint16_t> m_triangles;
};

}  // namespace gazeora
