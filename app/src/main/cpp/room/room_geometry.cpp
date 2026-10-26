#include "room/room_geometry.h"

namespace gazeora {

namespace {

constexpr float kMetersPerInch = 0.0254f;
constexpr int kFullTurn = 360;
constexpr int kQuarterTurn = 90;
constexpr float kAspectTolerance = 0.02f;

}  // namespace

namespace room_geometry {

float TanPerUnit(int32_t screenHeightPixels, float dotsPerInch) {
    float height = dotsPerInch > 0 ? screenHeightPixels / dotsPerInch * kMetersPerInch : 0;
    if (height < kMinScreenHeight || height > kMaxScreenHeight) {
        height = kTypicalScreenHeight;  // some phones report a made-up density
    }
    return height / 2 / kScreenToLens;
}

}  // namespace room_geometry

namespace camera_geometry {

Size PickStreamSize(const std::vector<Size>& sizes, float activeAspect, int32_t maxWidth) {
    Size sameShape, anyShape, smallest;
    for (const Size& size : sizes) {
        if (size.width <= 0 || size.height <= 0) continue;
        if (!smallest.width || size.width < smallest.width) smallest = size;
        if (size.width > maxWidth) continue;
        float aspect = float(size.width) / size.height;
        if (size.width > anyShape.width) anyShape = size;
        if (aspect > activeAspect * (1 - kAspectTolerance) &&
            aspect < activeAspect * (1 + kAspectTolerance) && size.width > sameShape.width) {
            sameShape = size;
        }
    }
    return sameShape.width ? sameShape : anyShape.width ? anyShape : smallest;
}

void SetAngles(CameraView& view, float focalLengthMm, float sensorWidthMm, float sensorHeightMm,
               int32_t pixelArrayWidth, int32_t pixelArrayHeight, int32_t activeWidth,
               int32_t activeHeight, int32_t outWidth, int32_t outHeight) {
    if (focalLengthMm <= 0 || sensorWidthMm <= 0 || sensorHeightMm <= 0 || pixelArrayWidth <= 0 ||
        pixelArrayHeight <= 0 || activeWidth <= 0 || activeHeight <= 0 || outWidth <= 0 ||
        outHeight <= 0) {
        return;
    }
    // The camera crops the active array's longer side to the stream's shape.
    float usedWidth = float(activeWidth), usedHeight = float(activeHeight);
    float outAspect = float(outWidth) / outHeight;
    if (outAspect > usedWidth / usedHeight) {
        usedHeight = usedWidth / outAspect;
    } else {
        usedWidth = usedHeight * outAspect;
    }
    float widthMm = sensorWidthMm * usedWidth / pixelArrayWidth;
    float heightMm = sensorHeightMm * usedHeight / pixelArrayHeight;
    view.tanAcross = widthMm / 2 / focalLengthMm;
    view.tanDown = heightMm / 2 / focalLengthMm;
}

void SetOrientation(CameraView& view, int32_t sensorOrientation, int quarterTurns) {
    // How far the sensor's picture turns clockwise to stand upright on this display, and
    // so how far upright coordinates turn back to the sensor's: (x, y) -> (-y, x) per
    // clockwise quarter turn.
    int upright = ((sensorOrientation - quarterTurns * kQuarterTurn) % kFullTurn + kFullTurn) %
                  kFullTurn / kQuarterTurn;
    int turns = (4 - upright) % 4;
    int m[2][2] = {{1, 0}, {0, 1}};
    for (int i = 0; i < turns; ++i) {
        int next[2][2] = {{-m[1][0], -m[1][1]}, {m[0][0], m[0][1]}};
        for (int r = 0; r < 2; ++r) {
            for (int c = 0; c < 2; ++c) m[r][c] = next[r][c];
        }
    }
    for (int r = 0; r < 2; ++r) {
        for (int c = 0; c < 2; ++c) view.toSensor[r][c] = m[r][c];
    }
}

}  // namespace camera_geometry

}  // namespace gazeora
