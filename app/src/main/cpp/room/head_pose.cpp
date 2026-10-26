#include "room/head_pose.h"

namespace gazeora {

namespace {

// Below this, the view is too near straight up or down to tell which way it faces.
constexpr float kMinLevelLength = 0.2f;

}  // namespace

namespace head_pose {

Mat3 DeviceToWorld(float x, float y, float z, float w) {
    // As SensorManager.getRotationMatrixFromVector.
    return {{{1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)},
             {2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)},
             {2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)}}};
}

Mat3 ScreenToDevice(int quarterTurns) {
    // The screen's axes in the device's. Turned 90 (the device a quarter turn to the left,
    // as SensorManager.remapCoordinateSystem(AXIS_Y, AXIS_MINUS_X) has it), the screen's
    // right is the device's bottom and its up the device's right.
    const Vec3 z = {0, 0, 1};
    switch (quarterTurns & 3) {
        case 1:
            return Mat3::FromColumns({0, -1, 0}, {1, 0, 0}, z);
        case 2:
            return Mat3::FromColumns({-1, 0, 0}, {0, -1, 0}, z);
        case 3:
            return Mat3::FromColumns({0, 1, 0}, {-1, 0, 0}, z);
        default:
            return Mat3();
    }
}

}  // namespace head_pose

void RoomAnchor::Place(const Mat3& screenToWorld) {
    const Vec3 up = {0, 0, 1};
    Vec3 view = screenToWorld.Apply({0, 0, -1});
    Vec3 level = {view.x, view.y, 0};
    if (level.Length() < kMinLevelLength) {
        // Looking down, the top of the head points ahead; looking up, behind.
        Vec3 top = screenToWorld.Apply({0, 1, 0});
        float ahead = view.z < 0 ? 1.0f : -1.0f;
        level = {top.x * ahead, top.y * ahead, 0};
    }
    Vec3 forward = level.Normalized();
    m_worldToRoom = Mat3::FromRows(forward.Cross(up), up, forward * -1.0f);
    m_placed = true;
}

Mat3 RoomAnchor::ScreenToRoom(const Mat3& screenToWorld) const {
    return m_placed ? m_worldToRoom * screenToWorld : Mat3();
}

}  // namespace gazeora
