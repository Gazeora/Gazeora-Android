#include <gtest/gtest.h>

#include <cmath>

#include "room/head_pose.h"
#include "room/room_geometry.h"

namespace gazeora {
namespace {

constexpr float kTolerance = 1e-5f;
constexpr float kPi = 3.14159265f;

void ExpectNear(const Vec3& actual, const Vec3& expected) {
    EXPECT_NEAR(actual.x, expected.x, kTolerance);
    EXPECT_NEAR(actual.y, expected.y, kTolerance);
    EXPECT_NEAR(actual.z, expected.z, kTolerance);
}

// A phone in landscape (display rotation 90: turned a quarter to the left), upright in front
// of the eyes, looking north. World: x east, y north, z up.
Mat3 LandscapeDeviceFacingNorth() {
    // The device's right edge points up, its top west, its screen south (at the eyes).
    return Mat3::FromColumns({0, 0, 1}, {-1, 0, 0}, {0, -1, 0});
}

Mat3 YawRight(float degrees) {
    float a = degrees * kPi / 180;
    // Turning right (clockwise seen from above) about the world's up.
    return Mat3::FromRows({std::cos(a), std::sin(a), 0}, {-std::sin(a), std::cos(a), 0}, {0, 0, 1});
}

TEST(HeadPoseTest, ReadsTheRotationVector) {
    ExpectNear(head_pose::DeviceToWorld(0, 0, 0, 1).Apply({1, 2, 3}), {1, 2, 3});
    // A quarter turn about up: the device's x points along the world's y.
    float s = std::sqrt(0.5f);
    ExpectNear(head_pose::DeviceToWorld(0, 0, s, s).Apply({1, 0, 0}), {0, 1, 0});
}

TEST(HeadPoseTest, TurnsTheScreenWithTheDisplay) {
    ExpectNear(head_pose::ScreenToDevice(0).Apply({1, 0, 0}), {1, 0, 0});
    // Turned to the left, the screen's right is the device's bottom, its up the device's right.
    ExpectNear(head_pose::ScreenToDevice(1).Apply({1, 0, 0}), {0, -1, 0});
    ExpectNear(head_pose::ScreenToDevice(1).Apply({0, 1, 0}), {1, 0, 0});
    ExpectNear(head_pose::ScreenToDevice(3).Apply({1, 0, 0}), {0, 1, 0});
    ExpectNear(head_pose::ScreenToDevice(2).Apply({0, 1, 0}), {0, -1, 0});
}

TEST(HeadPoseTest, LooksWhereTheBackCameraDoes) {
    Mat3 head = LandscapeDeviceFacingNorth() * head_pose::ScreenToDevice(1);
    ExpectNear(head.Apply({0, 0, -1}), {0, 1, 0});  // north
    ExpectNear(head.Apply({1, 0, 0}), {1, 0, 0});   // the screen's right is east
    ExpectNear(head.Apply({0, 1, 0}), {0, 0, 1});   // and its up is up
}

TEST(RoomAnchorTest, PutsTheWindowAheadAndKeepsItThere) {
    Mat3 head = LandscapeDeviceFacingNorth() * head_pose::ScreenToDevice(1);
    RoomAnchor anchor;
    ExpectNear(anchor.ScreenToRoom(head).Apply({0, 0, -1}), {0, 0, -1});  // not placed: ahead
    anchor.Place(head);
    ExpectNear(anchor.ScreenToRoom(head).Apply({0, 0, -1}), {0, 0, -1});
    ExpectNear(anchor.ScreenToRoom(head).Apply({0, 1, 0}), {0, 1, 0});

    // Turned 30 degrees right, the view points right of the window, which stays put.
    Mat3 turned = YawRight(30) * head;
    Vec3 view = anchor.ScreenToRoom(turned).Apply({0, 0, -1});
    EXPECT_NEAR(view.x, 0.5f, kTolerance);
    EXPECT_NEAR(view.z, -std::sqrt(0.75f), kTolerance);
}

TEST(RoomAnchorTest, StaysLevelWhenPlacedLookingDown) {
    // Looking 60 degrees down at the north: the room still faces north, level.
    float a = 60 * kPi / 180;
    Mat3 pitch =
        Mat3::FromRows({1, 0, 0}, {0, std::cos(a), std::sin(a)}, {0, -std::sin(a), std::cos(a)});
    Mat3 head = pitch * LandscapeDeviceFacingNorth() * head_pose::ScreenToDevice(1);
    EXPECT_LT(head.Apply({0, 0, -1}).z, -0.8f);
    RoomAnchor anchor;
    anchor.Place(head);
    Mat3 level = LandscapeDeviceFacingNorth() * head_pose::ScreenToDevice(1);
    ExpectNear(anchor.ScreenToRoom(level).Apply({0, 0, -1}), {0, 0, -1});

    // Straight down: the top of the head says which way is ahead.
    Mat3 down = Mat3::FromColumns({1, 0, 0}, {0, 1, 0}, {0, 0, 1});  // screen faces up
    anchor.Place(down);
    ExpectNear(anchor.ScreenToRoom(level).Apply({0, 0, -1}), {0, 0, -1});
}

TEST(CameraGeometryTest, TurnsTheViewOntoTheSensor) {
    CameraView view;
    // The usual back camera (mounted at 90) on a display turned left: already upright.
    camera_geometry::SetOrientation(view, 90, 1);
    EXPECT_EQ(view.toSensor[0][0], 1);
    EXPECT_EQ(view.toSensor[1][1], 1);
    // Turned right instead: upside down.
    camera_geometry::SetOrientation(view, 90, 3);
    EXPECT_EQ(view.toSensor[0][0], -1);
    EXPECT_EQ(view.toSensor[1][1], -1);
    // In portrait the sensor's picture turns a quarter clockwise to stand up, so the view's
    // right is the sensor's up (-y) and its down the sensor's right.
    camera_geometry::SetOrientation(view, 90, 0);
    EXPECT_EQ(view.toSensor[0][0], 0);
    EXPECT_EQ(view.toSensor[1][0], -1);
    EXPECT_EQ(view.toSensor[0][1], 1);
    EXPECT_EQ(view.toSensor[1][1], 0);
}

TEST(CameraGeometryTest, MeasuresTheViewOfTheStream) {
    CameraView view;
    // A 4:3 sensor, 6.4 x 4.8 mm, 4.8 mm lens, streaming 16:9: the height is cropped.
    camera_geometry::SetAngles(view, 4.8f, 6.4f, 4.8f, 4000, 3000, 4000, 3000, 1920, 1080);
    EXPECT_NEAR(view.tanAcross, 3.2f / 4.8f, kTolerance);
    EXPECT_NEAR(view.tanDown, 3.2f / 4.8f * 9 / 16, kTolerance);
    // Missing data leaves it as it was.
    camera_geometry::SetAngles(view, 0, 6.4f, 4.8f, 4000, 3000, 4000, 3000, 1920, 1080);
    EXPECT_NEAR(view.tanAcross, 3.2f / 4.8f, kTolerance);
}

TEST(CameraGeometryTest, PicksAStreamThatSeesTheWholeSensor) {
    using camera_geometry::Size;
    std::vector<Size> sizes = {{4000, 3000}, {1920, 1080}, {1440, 1080}, {1280, 960}, {640, 480}};
    Size picked = camera_geometry::PickStreamSize(sizes, 4.0f / 3, 1920);
    EXPECT_EQ(picked.width, 1440);
    EXPECT_EQ(picked.height, 1080);
    // No 4:3 small enough: the largest that is.
    picked =
        camera_geometry::PickStreamSize({{4000, 3000}, {1920, 1080}, {1280, 720}}, 4.0f / 3, 1920);
    EXPECT_EQ(picked.width, 1920);
    // Only big ones: the smallest of them.
    picked = camera_geometry::PickStreamSize({{4000, 3000}, {3840, 2160}}, 4.0f / 3, 1920);
    EXPECT_EQ(picked.width, 3840);
    EXPECT_EQ(camera_geometry::PickStreamSize({}, 4.0f / 3, 1920).width, 0);
}

TEST(RoomGeometryTest, ReachesThroughTheLens) {
    // No distortion, centered: the edges are one unit down and the eye's aspect across.
    room_geometry::EyeReach flat = room_geometry::ReachOfEye(0, 0, 1.2f, 0.5f, 0.8f);
    EXPECT_NEAR(flat.across, 1.2f * 0.8f, kTolerance);
    EXPECT_NEAR(flat.down, 0.8f, kTolerance);
    // The lens pushes them out; an off-center lens sees farther on its far side.
    room_geometry::EyeReach lens = room_geometry::ReachOfEye(0.34f, 0.55f, 1.0f, 0.4f, 1.0f);
    EXPECT_NEAR(lens.down, 1 + 0.34f + 0.55f, kTolerance);
    float side = 1.2f;
    EXPECT_NEAR(lens.across, side * (1 + 0.34f * side * side + 0.55f * side * side * side * side),
                1e-4f);
}

TEST(CameraGeometryTest, StretchesTheCameraEvenlyToFillTheEye) {
    CameraView view;
    view.tanAcross = 0.6f;
    view.tanDown = 0.45f;
    camera_geometry::SetOrientation(view, 90, 1);  // the sensor's rows run across
    room_geometry::EyeReach reach;
    reach.across = 1.2f;
    reach.down = 1.8f;  // down needs the most: four times
    camera_geometry::FillEye(view, reach);
    EXPECT_NEAR(view.tanAcross, 2.4f, kTolerance);
    EXPECT_NEAR(view.tanDown, 1.8f, kTolerance);

    // Turned a quarter, the sensor's rows run down the view.
    CameraView turned;
    turned.tanAcross = 0.6f;
    turned.tanDown = 0.45f;
    camera_geometry::SetOrientation(turned, 90, 0);
    reach.across = 0.9f;  // across is the sensor's down: twice
    reach.down = 0.6f;
    camera_geometry::FillEye(turned, reach);
    EXPECT_NEAR(turned.tanAcross, 1.2f, kTolerance);
    EXPECT_NEAR(turned.tanDown, 0.9f, kTolerance);

    // Wide enough already: true size.
    CameraView wide;
    wide.tanAcross = 3;
    wide.tanDown = 3;
    camera_geometry::FillEye(wide, reach);
    EXPECT_NEAR(wide.tanAcross, 3, kTolerance);
}

TEST(RoomGeometryTest, MeasuresTheEyesFromTheScreen) {
    // 1080 pixels at 400 dpi: 68.6 mm tall.
    EXPECT_NEAR(room_geometry::TanPerUnit(1080, 400), 1080 / 400.0f * 0.0254f / 2 / 0.039f,
                kTolerance);
    // A made-up density: a typical phone instead.
    EXPECT_NEAR(room_geometry::TanPerUnit(1080, 160), 0.0625f / 2 / 0.039f, kTolerance);
    EXPECT_NEAR(room_geometry::TanPerUnit(1080, 0), 0.0625f / 2 / 0.039f, kTolerance);
}

}  // namespace
}  // namespace gazeora
