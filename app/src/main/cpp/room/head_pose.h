#pragma once

#include "room/mat3.h"

namespace gazeora {

// Where the head points, from the phone's rotation sensor. Three frames are involved:
//  - the device's (Android sensor axes): x right and y up in the phone's natural (portrait)
//    orientation, z out of the screen;
//  - the screen's, as the viewer sees it in landscape: x right, y up, z towards the eyes, so
//    the view (and the back camera) looks down -z;
//  - the world's, from the sensor: z up against gravity, x and y level.
namespace head_pose {

// The device-to-world rotation of a rotation vector sensor's unit quaternion.
Mat3 DeviceToWorld(float x, float y, float z, float w);

// Screen-to-device for the display turned `quarterTurns` (Surface.ROTATION_*, 0..3) from
// the device's natural orientation.
Mat3 ScreenToDevice(int quarterTurns);

}  // namespace head_pose

// The room the window floats in: level, with the window straight ahead along -z. Placed
// where the head looks when the window appears; after that it stays put as the head turns.
// Room frame: x right, y up, z back towards the viewer.
class RoomAnchor {
public:
    // Faces the room where the head (screen-to-world) points, leveled.
    void Place(const Mat3& screenToWorld);
    bool Placed() const { return m_placed; }
    void Clear() { m_placed = false; }
    // Screen-to-room for the head now; the screen itself until placed.
    Mat3 ScreenToRoom(const Mat3& screenToWorld) const;

private:
    Mat3 m_worldToRoom;
    bool m_placed = false;
};

}  // namespace gazeora
