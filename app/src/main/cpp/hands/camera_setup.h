#pragma once

#include <camera/NdkCameraMetadata.h>

#include <cstdint>

#include "hands/view_geometry.h"

namespace gazeora {

// How to run a back camera for hand tracking, from its characteristics.
struct CameraSetup {
    camera_geometry::Size size;      // the small YUV stream to ask for
    int32_t sensorOrientation = 90;  // ACAMERA_SENSOR_ORIENTATION, degrees
    // Below 1, zoomed out (to the ultra-wide, on phones with one): hands show farther out to
    // the sides. 1 where the camera can't (or before Android 11).
    float zoomRatio = 1.0f;
    CameraView view;  // its angles at that zoom; the orientation is set per display
    // The frame rates to ask for; 0 to 0 to leave the camera to choose.
    camera_geometry::FrameRates frameRates;

    // False if it isn't a back camera, or streams no YUV.
    bool Read(const ACameraMetadata* characteristics);
};

}  // namespace gazeora
