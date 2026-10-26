#pragma once

#include <camera/NdkCameraMetadata.h>

#include <cstdint>

#include "room/room_geometry.h"

namespace gazeora {

// How to run a back camera for the room view, from its characteristics.
struct CameraSetup {
    camera_geometry::Size size;      // the stream to ask for
    int32_t sensorOrientation = 90;  // ACAMERA_SENSOR_ORIENTATION, degrees
    // Below 1, zoomed out (to the ultra-wide, on phones with one): the room around the window
    // shows wider. 1 where the camera can't (or before Android 11).
    float zoomRatio = 1.0f;
    CameraView view;  // its angles at that zoom; the orientation is set per display

    // False if it isn't a back camera, or streams nothing the renderer can sample.
    bool Read(const ACameraMetadata* characteristics);
};

}  // namespace gazeora
