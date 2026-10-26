#pragma once

#include <cstdint>
#include <vector>

namespace gazeora {

// The room view's sizes, in meters.
namespace room_geometry {

// Gazeora's window: this wide, its height from the picture's shape, this far ahead.
constexpr float kWindowWidth = 2.4f;
constexpr float kWindowDistance = 2.0f;
constexpr float kWindowCornerRadius = 0.05f;
// Between the eyes: each eye's ray starts this far apart, which puts the window at its depth.
constexpr float kEyeSeparation = 0.064f;
// From the phone's screen to the viewer's lenses (Google Cardboard 2015).
constexpr float kScreenToLens = 0.039f;
// A phone's screen height in landscape when its density can't be trusted.
constexpr float kTypicalScreenHeight = 0.0625f;
constexpr float kMinScreenHeight = 0.04f;
constexpr float kMaxScreenHeight = 0.12f;

// The tangent of the view angle at one unit from an eye's lens center, the unit the lens
// shader measures in: half the eye's height, which is half the screen's.
float TanPerUnit(int32_t screenHeightPixels, float dotsPerInch);

}  // namespace room_geometry

// The back camera's view: the tangents of half its angle along the sensor's rows and
// columns, and how to turn the screen's view onto the sensor.
struct CameraView {
    float tanAcross = 0.62f;  // about 64 degrees, a typical main camera, until it's known
    float tanDown = 0.47f;
    // Row-major 2x2 integer rotation: upright image coordinates (x right, y down, as seen on
    // the screen) to the sensor's.
    int toSensor[2][2] = {{1, 0}, {0, 1}};
};

namespace camera_geometry {

struct Size {
    int32_t width = 0;
    int32_t height = 0;
};

// The camera stream to ask for, out of `sizes`: the largest up to maxWidth with the active
// array's shape (so it sees all the sensor does), else the largest up to maxWidth, else the
// smallest. 0 x 0 if there are none.
Size PickStreamSize(const std::vector<Size>& sizes, float activeAspect, int32_t maxWidth);

// The part of the sensor a stream of outWidth x outHeight sees (the active array, cropped
// to the stream's shape), as tangents. Falls back to `view`'s when something is missing.
void SetAngles(CameraView& view, float focalLengthMm, float sensorWidthMm, float sensorHeightMm,
               int32_t pixelArrayWidth, int32_t pixelArrayHeight, int32_t activeWidth,
               int32_t activeHeight, int32_t outWidth, int32_t outHeight);

// For a back camera mounted `sensorOrientation` degrees (ACAMERA_SENSOR_ORIENTATION) on a
// display turned `quarterTurns` (Surface.ROTATION_*).
void SetOrientation(CameraView& view, int32_t sensorOrientation, int quarterTurns);

}  // namespace camera_geometry

}  // namespace gazeora
