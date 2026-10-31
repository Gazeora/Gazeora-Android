#pragma once

#include "hands/camera_frame.h"
#include "hands/vec3.h"

namespace gazeora {

// The room's light as the back camera sees it, to light the drawn hands the same way: where
// the main light comes from, its color and strength, and how much light there is all round.
struct SceneLight {
    // Toward the light, from the hands, in the camera's space (x right, y down, z ahead):
    // until the camera says otherwise, from above and a little in front, as a ceiling's.
    Vec3 from = {-0.25f, -0.8f, -0.55f};
    float color[3] = {1, 1, 1};  // the main light's, 0 to 1
    float strength = 1;          // the main light's
    float ambient = 0.35f;       // all round
};

namespace light_estimate {

// The light in an upright camera picture that spans `tanAcross` x `tanDown` (half its
// angles). A bright spot in view, as a lamp or a window, is the light; else the side of the
// picture that's brighter is the side it comes from. Its color is the picture's, softened.
SceneLight Estimate(const RgbaFrame& frame, float tanAcross, float tanDown);

// `from` moved `share` of the way (0 to 1) to `to`.
SceneLight Blend(const SceneLight& from, const SceneLight& to, float share);

}  // namespace light_estimate

}  // namespace gazeora
