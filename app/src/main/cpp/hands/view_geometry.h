#pragma once

#include <cstdint>
#include <vector>

namespace gazeora {

// What an eye sees through a VR viewer's lens; sizes in meters.
namespace eye_geometry {

// From the phone's screen to the viewer's lenses (Google Cardboard 2015).
constexpr float kScreenToLens = 0.039f;
// A phone's screen height in landscape when its density can't be trusted.
constexpr float kTypicalScreenHeight = 0.0625f;
constexpr float kMinScreenHeight = 0.04f;
constexpr float kMaxScreenHeight = 0.12f;

// The tangent of the view angle at one unit from an eye's lens center, the unit the lens
// shader measures in: half the eye's height, which is half the screen's.
float TanPerUnit(int32_t screenHeightPixels, float dotsPerInch);

}  // namespace eye_geometry

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

// Frames a second, from the least the camera may slow to (to take longer over each in dim
// light) to the most.
struct FrameRates {
    int32_t least = 0;
    int32_t most = 0;
};

// The frame rates to ask for, out of those the camera has (ACAMERA_CONTROL_AE_AVAILABLE_
// TARGET_FPS_RANGES): the fastest up to `most`, and of those the one that may slow nearest
// to `least` in dim light without going under it: slower, each picture takes in more light,
// so the hand tracker still sees hands in a dim room. 0 to 0 if there are none.
FrameRates PickFrameRates(const std::vector<FrameRates>& rates, int32_t least, int32_t most);

// The part of the sensor a stream of outWidth x outHeight sees (the active array, cropped
// to the stream's shape), as tangents. Falls back to `view`'s when something is missing.
void SetAngles(CameraView& view, float focalLengthMm, float sensorWidthMm, float sensorHeightMm,
               int32_t pixelArrayWidth, int32_t pixelArrayHeight, int32_t activeWidth,
               int32_t activeHeight, int32_t outWidth, int32_t outHeight);

// For a back camera mounted `sensorOrientation` degrees (ACAMERA_SENSOR_ORIENTATION) on a
// display turned `quarterTurns` (Surface.ROTATION_*).
void SetOrientation(CameraView& view, int32_t sensorOrientation, int quarterTurns);

// The camera's tangents of half its view as the screen shows it (after SetOrientation):
// across and down the upright picture.
void UprightTangents(const CameraView& view, float& across, float& down);

}  // namespace camera_geometry

}  // namespace gazeora
