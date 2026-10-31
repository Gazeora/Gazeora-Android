#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <vector>

#include "hands/camera_frame.h"
#include "hands/hand_detection.h"
#include "hands/hand_follower.h"
#include "hands/hand_geometry.h"
#include "hands/hand_model_data.h"
#include "hands/hand_pointer.h"
#include "hands/hand_shapes.h"
#include "hands/hand_skeleton.h"
#include "hands/hand_tracking.h"
#include "hands/light_estimate.h"
#include "hands/rigged_hand.h"
#include "hands/view_geometry.h"

namespace gazeora {
namespace {

constexpr float kTolerance = 1e-5f;

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

TEST(CameraGeometryTest, PicksFrameRatesThatSlowInTheDark) {
    using camera_geometry::FrameRates;
    // A typical camera's: the fastest up to 30, slowing to 15 in dim light but no further.
    FrameRates picked = camera_geometry::PickFrameRates(
        {{15, 15}, {7, 30}, {15, 30}, {24, 30}, {30, 60}}, 15, 30);
    EXPECT_EQ(picked.least, 15);
    EXPECT_EQ(picked.most, 30);
    // Without that, the nearest over it; or under it, if that's all there is.
    EXPECT_EQ(camera_geometry::PickFrameRates({{7, 30}, {24, 30}, {30, 30}}, 15, 30).least, 24);
    EXPECT_EQ(camera_geometry::PickFrameRates({{7, 30}, {10, 30}}, 15, 30).least, 10);
    // Nothing up to 30: the camera chooses.
    EXPECT_EQ(camera_geometry::PickFrameRates({{60, 60}}, 15, 30).most, 0);
    EXPECT_EQ(camera_geometry::PickFrameRates({}, 15, 30).most, 0);
}

TEST(EyeGeometryTest, MeasuresTheEyesFromTheScreen) {
    // 1080 pixels at 400 dpi: 68.6 mm tall.
    EXPECT_NEAR(eye_geometry::TanPerUnit(1080, 400), 1080 / 400.0f * 0.0254f / 2 / 0.039f,
                kTolerance);
    // A made-up density: a typical phone instead.
    EXPECT_NEAR(eye_geometry::TanPerUnit(1080, 160), 0.0625f / 2 / 0.039f, kTolerance);
    EXPECT_NEAR(eye_geometry::TanPerUnit(1080, 0), 0.0625f / 2 / 0.039f, kTolerance);
}

TEST(CameraGeometryTest, GivesTheTangentsOfTheUprightPicture) {
    CameraView view;
    view.tanAcross = 0.6f;
    view.tanDown = 0.45f;
    float across = 0, down = 0;
    camera_geometry::SetOrientation(view, 90, 1);
    camera_geometry::UprightTangents(view, across, down);
    EXPECT_NEAR(across, 0.6f, kTolerance);
    EXPECT_NEAR(down, 0.45f, kTolerance);
    camera_geometry::SetOrientation(view, 90, 0);  // a quarter turn swaps them
    camera_geometry::UprightTangents(view, across, down);
    EXPECT_NEAR(across, 0.45f, kTolerance);
    EXPECT_NEAR(down, 0.6f, kTolerance);
}

// A gray 3 x 2 picture whose luma counts up along its rows: 0 1 2 / 3 4 5.
struct GrayPicture {
    uint8_t luma[6] = {0, 1, 2, 3, 4, 5};
    uint8_t chroma[2] = {128, 128};
    YuvPlanes planes;

    GrayPicture() {
        planes.y = luma;
        planes.u = chroma;
        planes.v = chroma;
        planes.width = 3;
        planes.height = 2;
        planes.yRowStride = 3;
        planes.uvRowStride = 2;
        planes.uvPixelStride = 1;
    }
};

std::vector<int> Reds(const RgbaFrame& frame) {
    std::vector<int> reds;
    for (size_t i = 0; i < frame.rgba.size(); i += 4) reds.push_back(frame.rgba[i]);
    return reds;
}

TEST(CameraFrameTest, StandsThePictureUprightAsTheEyesShowIt) {
    GrayPicture picture;
    CameraView view;
    RgbaFrame frame;
    camera_geometry::SetOrientation(view, 90, 1);  // already upright
    camera_frame::UprightRgba(picture.planes, view.toSensor, frame);
    EXPECT_EQ(frame.width, 3);
    EXPECT_EQ(frame.height, 2);
    EXPECT_EQ(Reds(frame), (std::vector<int>{0, 1, 2, 3, 4, 5}));
    EXPECT_EQ(frame.rgba[3], 255);

    camera_geometry::SetOrientation(view, 90, 3);  // upside down
    camera_frame::UprightRgba(picture.planes, view.toSensor, frame);
    EXPECT_EQ(Reds(frame), (std::vector<int>{5, 4, 3, 2, 1, 0}));

    // In portrait, the upright picture's top left is the sensor's bottom left (toSensor takes
    // (-1, -1) to (-1, 1)).
    camera_geometry::SetOrientation(view, 90, 0);
    camera_frame::UprightRgba(picture.planes, view.toSensor, frame);
    EXPECT_EQ(frame.width, 2);
    EXPECT_EQ(frame.height, 3);
    EXPECT_EQ(Reds(frame), (std::vector<int>{3, 0, 4, 1, 5, 2}));
}

TEST(CameraFrameTest, ConvertsTheColors) {
    uint8_t luma = 128, u = 128, v = 255;
    YuvPlanes planes;
    planes.y = &luma;
    planes.u = &u;
    planes.v = &v;
    planes.width = planes.height = 1;
    planes.yRowStride = planes.uvRowStride = planes.uvPixelStride = 1;
    CameraView view;
    RgbaFrame frame;
    camera_frame::UprightRgba(planes, view.toSensor, frame);
    EXPECT_EQ(frame.rgba[0], 255);  // red, clamped
    EXPECT_EQ(frame.rgba[1], 128 - 183 * 127 / 256);
    EXPECT_EQ(frame.rgba[2], 128);
}

// A gray 4 x 4 picture whose luma rises along its rows, row by row: 0, 10, ... 150.
struct GrayRamp {
    uint8_t luma[16];
    uint8_t chroma[4] = {128, 128, 128, 128};
    YuvPlanes planes;
    GrayRamp() {
        for (int i = 0; i < 16; ++i) luma[i] = uint8_t(10 * i);
        planes.y = luma;
        planes.u = planes.v = chroma;
        planes.width = planes.height = 4;
        planes.yRowStride = 4;
        planes.uvRowStride = 2;
        planes.uvPixelStride = 1;
    }
};

TEST(CameraFrameTest, CropsATurnedSquare) {
    GrayRamp ramp;
    const int upright[2][2] = {{1, 0}, {0, 1}};
    float rgb[4 * 4 * 3];
    // The whole picture, as it is: each pixel's own luma, gray.
    camera_frame::CropRgb(ramp.planes, upright, {2, 2, 4, 0}, 4, 1, rgb);
    for (int i = 0; i < 16; ++i) {
        EXPECT_NEAR(rgb[3 * i], 10 * i / 255.0f, 1e-5f) << "pixel " << i;
        EXPECT_NEAR(rgb[3 * i + 1], rgb[3 * i], 1e-5f);
    }
    // Turned a quarter clockwise: its top row runs down the picture's last column.
    camera_frame::CropRgb(ramp.planes, upright, {2, 2, 4, 1.5707964f}, 4, 1, rgb);
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            EXPECT_NEAR(rgb[3 * (4 * row + column)], ramp.luma[4 * column + 3 - row] / 255.0f,
                        1e-4f);
        }
    }
    // Brightened, as far as white; black outside the picture.
    camera_frame::CropRgb(ramp.planes, upright, {2, 2, 4, 0}, 4, 2, rgb);
    EXPECT_NEAR(rgb[3 * 5], 100 / 255.0f, 1e-5f);
    EXPECT_EQ(rgb[3 * 15], 1.0f);
    camera_frame::CropRgb(ramp.planes, upright, {4, 2, 4, 0}, 4, 1, rgb);
    EXPECT_EQ(rgb[3 * 2], 0.0f);
    EXPECT_NEAR(rgb[0], 20 / 255.0f, 1e-5f);
    // And on the sensor turned upside down, the same square of the upright picture.
    const int flipped[2][2] = {{-1, 0}, {0, -1}};
    camera_frame::CropRgb(ramp.planes, flipped, {2, 2, 4, 0}, 4, 1, rgb);
    EXPECT_NEAR(rgb[0], 150 / 255.0f, 1e-5f);
}

TEST(CameraFrameTest, MeasuresHowBrightASquareIs) {
    GrayRamp ramp;
    const int upright[2][2] = {{1, 0}, {0, 1}};
    EXPECT_NEAR(camera_frame::MeanLuma(ramp.planes, upright, {2, 2, 4, 0}), 75 / 255.0f, 1e-3f);
    // The top left pixel alone.
    EXPECT_NEAR(camera_frame::MeanLuma(ramp.planes, upright, {0.5f, 0.5f, 1, 0}), 0, 1e-6f);
    EXPECT_EQ(camera_frame::MeanLuma(ramp.planes, upright, {50, 50, 4, 0}), 0);
}

// A hand in the upright picture: the wrist at (x, y), each finger's base `palm` out from it
// (the middle one straight ahead), pointing `angle` radians clockwise from straight up.
HandLandmarks Hand(float x, float y, float palm, float angle) {
    HandLandmarks hand;
    const float dx = std::sin(angle), dy = -std::cos(angle);
    for (int i = 0; i < HandLandmarks::kCount; ++i) {
        // Out along the hand, and across it a little so the knuckles have their width.
        float along = palm * (i == 0 ? 0 : 1 + 0.1f * ((i - 1) % 4));
        float side = i == 0 ? 0 : palm * 0.2f * ((i - 1) / 4 - 2);
        hand.image[i][0] = x + dx * along - dy * side;
        hand.image[i][1] = y + dy * along + dx * side;
    }
    return hand;
}

// The test camera sees 0.5 each way, so the picture spans -0.5..0.5 in tangents, its size.
constexpr float kCameraTan = 0.5f;

ShownHand Seen(float x, float y, float palm, float angle) {
    return hand_geometry::Show(Hand(x, y, palm, angle), kCameraTan, kCameraTan);
}

TEST(HandGeometryTest, ShowsLandmarksWhereTheCameraSeesThem) {
    HandLandmarks hand;
    hand.image[0][0] = 0.5f;
    hand.image[0][1] = 0.5f;
    hand.image[1][0] = 1.0f;
    hand.image[1][1] = 0.0f;
    ShownHand shown = hand_geometry::Show(hand, 0.8f, 0.6f);
    EXPECT_NEAR(shown.points[0].x, 0, kTolerance);
    EXPECT_NEAR(shown.points[0].y, 0, kTolerance);
    EXPECT_NEAR(shown.points[1].x, 0.8f, kTolerance);   // the right edge
    EXPECT_NEAR(shown.points[1].y, -0.6f, kTolerance);  // the top
}

TEST(HandGeometryTest, MeasuresThePalm) {
    EXPECT_NEAR(Seen(0.5f, 0.8f, 0.2f, 0.3f).palm, 0.2f, 1e-4f);
    // Fingers pointing away: wrist and middle knuckle almost meet, but the knuckles keep their
    // width.
    HandLandmarks hand = Hand(0.5f, 0.6f, 0.2f, 0);
    hand.image[HandLandmarks::kMiddleBase][0] = 0.5f;
    hand.image[HandLandmarks::kMiddleBase][1] = 0.58f;
    EXPECT_GT(hand_geometry::Show(hand, kCameraTan, kCameraTan).palm, 0.1f);
}

TEST(HandGeometryTest, KnowsWhatsWithinReach) {
    // A palm 0.2 across in tangents is about 45 cm off; 0.05 is 1.8 m off.
    EXPECT_TRUE(hand_geometry::WithinReach(Seen(0.5f, 0.8f, 0.2f, 0)));
    EXPECT_FALSE(hand_geometry::WithinReach(Seen(0.5f, 0.8f, 0.05f, 0)));
}

TEST(HandGeometryTest, TellsTheViewersHandsFromOthers) {
    using hand_geometry::ComesFromViewer;
    // Raised from below: the arm goes out of view at the bottom.
    EXPECT_TRUE(ComesFromViewer(Seen(0.6f, 0.9f, 0.2f, 0.2f), kCameraTan, kCameraTan));
    // Reaching in from the side.
    EXPECT_TRUE(ComesFromViewer(Seen(0.05f, 0.5f, 0.2f, 0.8f), kCameraTan, kCameraTan));
    // Someone across from the viewer waving: their forearm shows below their hand.
    EXPECT_FALSE(ComesFromViewer(Seen(0.5f, 0.45f, 0.15f, 0), kCameraTan, kCameraTan));
    // Someone handing the viewer something: fingers pointing down at them.
    EXPECT_FALSE(ComesFromViewer(Seen(0.5f, 0.2f, 0.2f, 3.0f), kCameraTan, kCameraTan));
    // Pointing to the side, a little down, is still the viewer's.
    EXPECT_TRUE(ComesFromViewer(Seen(0.5f, 0.6f, 0.2f, 1.8f), kCameraTan, kCameraTan));
}

TEST(HandFollowerTest, TakesOnlyTheViewersHands) {
    HandFollower follower;
    // The viewer's hand, from below, and someone else's, waving in the middle.
    follower.Update({Seen(0.7f, 0.9f, 0.2f, 0), Seen(0.4f, 0.45f, 0.15f, 0)}, kCameraTan,
                    kCameraTan, 0);
    std::vector<ShownHand> hands = follower.Hands(0);
    ASSERT_EQ(hands.size(), 1u);
    EXPECT_NEAR(hands[0].points[HandLandmarks::kWrist].x, 0.2f, kTolerance);
}

TEST(HandFollowerTest, FollowsTheViewersHandWhereverItGoes) {
    HandFollower follower;
    float y = 0.9f;
    follower.Update({Seen(0.5f, y, 0.2f, 0)}, kCameraTan, kCameraTan, 0);
    // Up into the middle of the view, where a hand just found wouldn't count: still followed.
    double now = 0;
    for (int step = 0; step < 25; ++step) {
        y -= 0.02f;
        now += 0.033;
        follower.Update({Seen(0.5f, y, 0.2f, 0)}, kCameraTan, kCameraTan, now);
    }
    EXPECT_FALSE(hand_geometry::ComesFromViewer(Seen(0.5f, y, 0.2f, 0), kCameraTan, kCameraTan));
    std::vector<ShownHand> hands = follower.Hands(now);
    ASSERT_EQ(hands.size(), 1u);
    EXPECT_NEAR(hands[0].points[HandLandmarks::kWrist].y, (2 * y - 1) * kCameraTan, 0.03f);
    // Lost for a moment: still there. Lost for longer: gone.
    follower.Update({}, kCameraTan, kCameraTan, now + 0.1);
    EXPECT_EQ(follower.Hands(now + 0.1).size(), 1u);
    EXPECT_TRUE(follower.Hands(now + 0.5).empty());
}

TEST(HandFollowerTest, KeepsTwoHandsAtMost) {
    HandFollower follower;
    follower.Update({Seen(0.2f, 0.9f, 0.2f, 0), Seen(0.8f, 0.9f, 0.25f, 0)}, kCameraTan,
                    kCameraTan, 0);
    // A third from below, while both are followed, doesn't take their place.
    follower.Update({Seen(0.2f, 0.9f, 0.2f, 0), Seen(0.8f, 0.9f, 0.25f, 0),
                     Seen(0.5f, 0.95f, 0.3f, 0)},
                    kCameraTan, kCameraTan, 0.033);
    std::vector<ShownHand> hands = follower.Hands(0.033);
    ASSERT_EQ(hands.size(), 2u);
    EXPECT_NEAR(hands[0].palm, 0.25f, 1e-3f);  // nearest first
    EXPECT_NEAR(hands[1].palm, 0.2f, 1e-3f);
}

TEST(HandFollowerTest, SteadiesAStillHand) {
    HandFollower follower;
    follower.Update({Seen(0.5f, 0.9f, 0.2f, 0)}, kCameraTan, kCameraTan, 0);
    // The tracker's answers wobble a little about where the hand is.
    double now = 0;
    float most = 0;
    for (int step = 0; step < 30; ++step) {
        now += 0.033;
        float wobble = step % 2 ? 0.01f : -0.01f;
        follower.Update({Seen(0.5f + wobble, 0.9f, 0.2f, 0)}, kCameraTan, kCameraTan, now);
        most = std::max(most, std::fabs(follower.Hands(now)[0].points[0].x));
    }
    EXPECT_LT(most, 0.01f * 2 * kCameraTan * 0.7f);  // it wobbles less than the answers
}

// A hand built from the skeleton's own measures (hand_skeleton.h), posed by joint angles and
// put in front of the camera: its 21 landmarks in meters, in the camera's space, as a hand
// that the skeleton can be exactly.
struct DesignHand {
    Vec3 wrist = {0.04f, 0.1f, 0.42f};
    // The palm's way along (wrist to knuckles) and across (toward the thumb); not yet square.
    Vec3 along = {0.1f, -1, 0.35f};
    Vec3 across = {-1, 0, 0.2f};
    bool right = true;
    // Each finger's knuckle, middle joint and tip bends (the thumb's first is its turn toward
    // the palm), and its spread from its rest.
    float bends[HandSkeleton::kFingers][3] = {};
    float spreads[HandSkeleton::kFingers] = {0, 0.12f, 0.03f, -0.06f, -0.18f};

    Axes Palm() const {
        Axes palm;
        palm.across = Normalize(across, Vec3{1, 0, 0});
        palm.along = Normalize(Flatten(along, palm.across), Vec3{0, -1, 0});
        palm.palmward = Cross(palm.across, palm.along) * (right ? 1.0f : -1.0f);
        return palm;
    }

    static Axes Bent(const Axes& a, float angle) {
        float c = std::cos(angle), s = std::sin(angle);
        return {a.along * c + a.palmward * s, a.palmward * c - a.along * s, a.across};
    }

    void Landmarks(Vec3 (&at)[HandLandmarks::kCount]) const {
        const Axes palm = Palm();
        at[0] = wrist;
        for (int f = 0; f < HandSkeleton::kFingers; ++f) {
            const float* k = hand_skeleton::kKnuckles[f];
            Vec3 joint = wrist + palm.along * k[0] + palm.across * k[1] + palm.palmward * k[2];
            Axes axes;
            if (f == 0) {
                // The thumb's rest way, as the skeleton has it, curling across the palm.
                const float* rest = hand_skeleton::kThumbRest;
                axes = hand_skeleton::ThumbAxes(
                    palm, Normalize(palm.along * rest[0] + palm.across * rest[1] +
                                        palm.palmward * rest[2],
                                    palm.along));
            } else {
                float c = std::cos(spreads[f]), s = std::sin(spreads[f]);
                axes = {palm.along * c + palm.across * s, palm.palmward,
                        palm.across * c - palm.along * s};
                axes = Bent(axes, bends[f][0]);
            }
            at[f * 4 + 1] = joint;
            for (int b = 0; b < 3; ++b) {
                if (b > 0 || f == 0) {
                    if (b > 0) axes = Bent(axes, bends[f][b]);
                }
                joint = joint + axes.along * hand_skeleton::kLengths[f][b];
                at[f * 4 + 2 + b] = joint;
            }
        }
    }

    // As hand_geometry::Show gives it, with its 3D shape.
    ShownHand Shown() const {
        Vec3 at[HandLandmarks::kCount];
        Landmarks(at);
        ShownHand hand;
        for (int i = 0; i < HandLandmarks::kCount; ++i) {
            hand.points[i] = {at[i].x / at[i].z, at[i].y / at[i].z};
            hand.depth[i] = at[i].z / wrist.z - 1;
        }
        hand.palm = hand_geometry::kPalmLength / wrist.z;
        hand.right = right;
        hand.solid = true;
        return hand;
    }

    // As the tracker gives it, for a camera `tan` wide each way.
    HandLandmarks Tracked(float tan) const {
        Vec3 at[HandLandmarks::kCount];
        Landmarks(at);
        Vec3 middle;
        for (const Vec3& p : at) middle = middle + p * (1.0f / HandLandmarks::kCount);
        HandLandmarks hand;
        for (int i = 0; i < HandLandmarks::kCount; ++i) {
            hand.image[i][0] = (at[i].x / at[i].z / tan + 1) / 2;
            hand.image[i][1] = (at[i].y / at[i].z / tan + 1) / 2;
            Vec3 shape = at[i] - middle;
            hand.shape[i][0] = shape.x;
            hand.shape[i][1] = shape.y;
            hand.shape[i][2] = shape.z;
        }
        hand.right = right;
        return hand;
    }
};

// A relaxed hand: fingers a little bent, the thumb a little curled.
DesignHand Relaxed() {
    DesignHand hand;
    const float bends[HandSkeleton::kFingers][3] = {
        {0, 0.3f, 0.25f}, {0.3f, 0.6f, 0.42f}, {0.35f, 0.65f, 0.455f}, {0.4f, 0.7f, 0.49f},
        {0.45f, 0.75f, 0.525f}};
    std::copy(&bends[0][0], &bends[0][0] + HandSkeleton::kFingers * 3, &hand.bends[0][0]);
    return hand;
}

void ExpectNear(Vec3 a, Vec3 b, float tolerance, const char* what) {
    EXPECT_NEAR(a.x, b.x, tolerance) << what;
    EXPECT_NEAR(a.y, b.y, tolerance) << what;
    EXPECT_NEAR(a.z, b.z, tolerance) << what;
}

// How far `b` bends from `a`, toward a's palmward side.
float BendBetween(const Bone& a, const Bone& b) {
    return std::atan2(Dot(b.axes.along, a.axes.palmward), Dot(b.axes.along, a.axes.along));
}

TEST(HandGeometryTest, PlacesTheHandsShapeWhereThePictureShowsIt) {
    const DesignHand design = Relaxed();
    ShownHand shown = hand_geometry::Show(design.Tracked(0.6f), 0.6f, 0.6f);
    ASSERT_TRUE(shown.solid);
    EXPECT_NEAR(shown.palm, hand_geometry::kPalmLength / design.wrist.z, 1e-4f);
    ShownHand exact = design.Shown();
    for (int i = 0; i < HandLandmarks::kCount; ++i) {
        EXPECT_NEAR(shown.depth[i], exact.depth[i], 1e-4f) << "landmark " << i;
    }
}

TEST(HandGeometryTest, GuessesWhenTheShapeDoesntFitThePicture) {
    HandLandmarks hand = Relaxed().Tracked(0.6f);
    // A shape the picture can't show: all one point.
    for (auto& point : hand.shape) point[0] = point[1] = point[2] = 0;
    ShownHand shown = hand_geometry::Show(hand, 0.6f, 0.6f);
    EXPECT_FALSE(shown.solid);
    EXPECT_GT(shown.palm, 0);  // from how big the palm looks
    for (float depth : shown.depth) EXPECT_EQ(depth, 0);
}

TEST(HandGeometryTest, TellsARightHandFromALeftByItsShape) {
    DesignHand right = Relaxed(), left = Relaxed();
    left.right = false;
    EXPECT_GT(hand_geometry::Rightness(right.Shown()), 0.1f);
    EXPECT_LT(hand_geometry::Rightness(left.Shown()), -0.1f);
}

TEST(HandFollowerTest, TellsARightHandFromALeftByWhereItIs) {
    // A right hand, on the right, that the tracker calls left.
    DesignHand design = Relaxed();
    design.wrist.x = 0.15f;
    HandLandmarks tracked = design.Tracked(0.6f);
    tracked.right = false;
    ShownHand seen = hand_geometry::Show(tracked, 0.6f, 0.6f);
    ASSERT_TRUE(seen.solid);
    HandFollower follower;
    double now = 0;
    // A hand only counts as the viewer's when it comes from them: say the camera sees little
    // down, so its arm is out of view below.
    follower.Update({seen}, 0.6f, 0.05f, now);
    for (int k = 0; k < 10; ++k) {
        now += 0.033;
        follower.Update({seen}, 0.6f, 0.05f, now);
    }
    std::vector<ShownHand> hands = follower.Hands(now);
    ASSERT_EQ(hands.size(), 1u);
    EXPECT_TRUE(hands[0].right);
    // Its shape is a right hand's still.
    for (int i = 0; i < HandLandmarks::kCount; ++i) {
        EXPECT_NEAR(hands[0].depth[i], seen.depth[i], 1e-5f) << "landmark " << i;
    }
}

TEST(HandFollowerTest, TurnsAHandTheTrackerTookForTheOtherBack) {
    // The right hand on the right, but taken for a left one: turned about, its depths reversed.
    DesignHand design = Relaxed();
    design.wrist.x = 0.15f;
    const ShownHand exact = design.Shown();
    ShownHand seen = exact;
    for (float& depth : seen.depth) depth = -depth;
    seen.right = false;
    ASSERT_LT(hand_geometry::Rightness(seen), 0);
    HandFollower follower;
    follower.Update({seen}, 0.6f, 0.05f, 0);
    const std::vector<ShownHand> hands = follower.Hands(0);
    ASSERT_EQ(hands.size(), 1u);
    EXPECT_TRUE(hands[0].right);
    for (int i = 0; i < HandLandmarks::kCount; ++i) {
        EXPECT_NEAR(hands[0].depth[i], exact.depth[i], 1e-5f) << "landmark " << i;
    }
}

TEST(HandFollowerTest, KeepsEachHandsSide) {
    HandFollower follower;
    // Its arm in from the left: a left hand, whatever the tracker says.
    ShownHand left = Seen(0.4f, 0.9f, 0.2f, 0.4f);
    left.right = true;
    follower.Update({left}, kCameraTan, kCameraTan, 0);
    ASSERT_EQ(follower.Hands(0).size(), 1u);
    EXPECT_FALSE(follower.Hands(0)[0].right);
    // The tracker changing its mind doesn't change it; another hand is the other.
    ShownHand other = Seen(0.8f, 0.9f, 0.2f, -0.4f);
    other.right = false;
    left.right = false;
    follower.Update({left, other}, kCameraTan, kCameraTan, 0.033);
    std::vector<ShownHand> hands = follower.Hands(0.033);
    ASSERT_EQ(hands.size(), 2u);
    EXPECT_NE(hands[0].right, hands[1].right);
    for (const ShownHand& hand : hands) {
        EXPECT_EQ(hand.right, hand.points[0].x > 0);
    }
}

TEST(HandSkeletonTest, FitsAHandItCanBeExactly) {
    const DesignHand design = Relaxed();
    Vec3 at[HandLandmarks::kCount];
    design.Landmarks(at);
    HandSkeleton skeleton = hand_skeleton::Fit(design.Shown());
    ExpectNear(skeleton.wrist, design.wrist, 1e-4f, "wrist");
    for (int f = 0; f < HandSkeleton::kFingers; ++f) {
        for (int b = 0; b < HandSkeleton::kBones; ++b) {
            const Bone& bone = skeleton.bones[f][b];
            ExpectNear(bone.start, at[f * 4 + 1 + b], 2e-4f, "joint");
            ExpectNear(bone.End(), at[f * 4 + 2 + b], 2e-4f, "next joint");
            EXPECT_NEAR(bone.length, hand_skeleton::kLengths[f][b], 1e-6f);
        }
    }
}

TEST(HandSkeletonTest, NeverBendsAFingerBackward) {
    // The tracker sees the index finger's middle joint bent well back, and its knuckle bent
    // back past what a knuckle can.
    DesignHand design = Relaxed();
    design.bends[1][0] = -0.9f;
    design.bends[1][1] = -0.8f;
    HandSkeleton skeleton = hand_skeleton::Fit(design.Shown());
    const Bone (&index)[HandSkeleton::kBones] = skeleton.bones[1];
    EXPECT_GE(BendBetween(index[0], index[1]), -1e-4f);
    float knuckle = std::atan2(Dot(index[0].axes.along, skeleton.palm.palmward),
                               Dot(index[0].axes.along, Normalize(Flatten(index[0].axes.along,
                                                                          skeleton.palm.palmward),
                                                                  skeleton.palm.along)));
    EXPECT_NEAR(knuckle, hand_skeleton::kKnuckleBack, 1e-3f);
}

TEST(HandSkeletonTest, KeepsItsBonesTheirLength) {
    // The tracker sees the fingers half as long again as they are.
    const DesignHand design = Relaxed();
    Vec3 at[HandLandmarks::kCount];
    design.Landmarks(at);
    for (int i = 2; i < HandLandmarks::kCount; ++i) {
        const int base = (i - 1) / 4 * 4 + 1;
        if (i != base) at[i] = at[base] + (at[i] - at[base]) * 1.5f;
    }
    ShownHand stretched = design.Shown();
    for (int i = 0; i < HandLandmarks::kCount; ++i) {
        stretched.points[i] = {at[i].x / at[i].z, at[i].y / at[i].z};
        stretched.depth[i] = at[i].z / design.wrist.z - 1;
    }
    HandSkeleton skeleton = hand_skeleton::Fit(stretched);
    for (int f = 0; f < HandSkeleton::kFingers; ++f) {
        for (int b = 1; b < HandSkeleton::kBones; ++b) {
            // Each joint where the one before ends: no bone stretches.
            ExpectNear(skeleton.bones[f][b].start, skeleton.bones[f][b - 1].End(), 1e-6f, "joint");
        }
        // And still bent as the tracker saw it.
        EXPECT_NEAR(BendBetween(skeleton.bones[f][0], skeleton.bones[f][1]), design.bends[f][1],
                    2e-3f);
    }
}

TEST(HandSkeletonTest, MirrorsTheRightHandForTheLeft) {
    DesignHand right = Relaxed();
    DesignHand left = right;
    left.right = false;
    left.wrist.x = -right.wrist.x;
    left.along.x = -right.along.x;
    left.across.x = -right.across.x;
    HandSkeleton r = hand_skeleton::Fit(right.Shown()), l = hand_skeleton::Fit(left.Shown());
    for (int f = 0; f < HandSkeleton::kFingers; ++f) {
        for (int b = 0; b < HandSkeleton::kBones; ++b) {
            Vec3 mirrored = r.bones[f][b].End();
            mirrored.x = -mirrored.x;
            ExpectNear(l.bones[f][b].End(), mirrored, 2e-4f, "mirrored joint");
        }
    }
}

TEST(RiggedHandTest, HasTheModelsSkin) {
    const RiggedHand& rig = RiggedHand::Get();
    EXPECT_EQ(rig.Corners(), std::size(hand_model::kCorners));
    const std::vector<uint16_t>& triangles = rig.Triangles();
    ASSERT_EQ(triangles.size() % 3, 0u);
    EXPECT_GT(triangles.size() / 3, 1000u);
    for (uint16_t corner : triangles) {
        ASSERT_LT(corner, rig.Corners());
    }
    // Its skeleton is the skeleton's own, joint for joint.
    const HandSkeleton& rest = rig.Rest();
    for (int f = 0; f < HandSkeleton::kFingers; ++f) {
        for (int b = 0; b < HandSkeleton::kBones; ++b) {
            EXPECT_NEAR(rest.bones[f][b].length, hand_skeleton::kLengths[f][b], 1e-4f);
        }
        const float* k = hand_skeleton::kKnuckles[f];
        ExpectNear(rest.bones[f][0].start,
                   rest.palm.along * k[0] + rest.palm.across * k[1] + rest.palm.palmward * k[2],
                   1e-4f, "knuckle");
    }
}

TEST(RiggedHandTest, FitsTheModelsOwnPose) {
    // The model's rest pose as the tracker would see it, ahead of the camera.
    const RiggedHand& rig = RiggedHand::Get();
    const Vec3 wrist = {0.03f, 0.08f, 0.4f};
    const Axes palm = {Normalize(Vec3{0.1f, -1, 0.3f}, Vec3{0, -1, 0}), {}, {}};
    Axes camera = palm;
    camera.across = Normalize(Flatten(Vec3{-1, 0, 0.1f}, palm.along), Vec3{-1, 0, 0});
    camera.palmward = Cross(camera.across, camera.along);
    auto place = [&](Vec3 p) {
        return wrist + camera.along * p.x + camera.palmward * -p.z + camera.across * p.y;
    };
    ShownHand shown;
    for (int i = 0; i < HandLandmarks::kCount; ++i) {
        const float* j = hand_model::kJoints[i];
        const Vec3 at = place({j[0], j[1], j[2]});
        shown.points[i] = {at.x / at.z, at.y / at.z};
        shown.depth[i] = at.z / wrist.z - 1;
    }
    shown.palm = hand_geometry::kPalmLength / wrist.z;
    shown.right = true;
    shown.solid = true;
    std::vector<HandVertex> rest, posed;
    rig.Pose(rig.Rest(), rest);
    rig.Pose(hand_skeleton::Fit(shown), posed);
    // The skin where the model has it, but for the fit's own smoothing of the fingertips.
    float worst = 0;
    for (size_t v = 0; v < posed.size(); ++v) {
        const Vec3 want = place({rest[v].x, rest[v].y, rest[v].z});
        worst = std::max(worst, Length(Vec3{posed[v].x, posed[v].y, posed[v].z} - want));
    }
    EXPECT_LT(worst, 0.006f);
}

// `p` turned about a fixed axis and moved, as a whole hand might be.
Vec3 Moved(Vec3 p, bool point = true) {
    const Vec3 axis = Normalize(Vec3{0.3f, 0.8f, -0.5f}, Vec3{0, 0, 1});
    const float c = std::cos(0.9f), s = std::sin(0.9f);
    const Vec3 turned = p * c + Cross(axis, p) * s + axis * (Dot(axis, p) * (1 - c));
    return point ? turned + Vec3{0.05f, -0.1f, 0.4f} : turned;
}

TEST(RiggedHandTest, MovesAsOneWithTheWholeSkeleton) {
    const RiggedHand& rig = RiggedHand::Get();
    HandSkeleton moved = rig.Rest();
    auto move = [](Axes& axes) {
        axes = {Moved(axes.along, false), Moved(axes.palmward, false), Moved(axes.across, false)};
    };
    moved.wrist = Moved(moved.wrist);
    move(moved.palm);
    for (auto& finger : moved.bones) {
        for (Bone& bone : finger) {
            bone.start = Moved(bone.start);
            move(bone.axes);
        }
    }
    std::vector<HandVertex> rest, posed;
    rig.Pose(rig.Rest(), rest);
    rig.Pose(moved, posed);
    ASSERT_EQ(posed.size(), rig.Corners());
    for (size_t v = 0; v < posed.size(); ++v) {
        ExpectNear(Vec3{posed[v].x, posed[v].y, posed[v].z},
                   Moved(Vec3{rest[v].x, rest[v].y, rest[v].z}), 1e-5f, "corner");
        ExpectNear(Vec3{posed[v].nx, posed[v].ny, posed[v].nz},
                   Moved(Vec3{rest[v].nx, rest[v].ny, rest[v].nz}, false), 1e-4f, "normal");
        EXPECT_GE(posed[v].shows, 0.0f);
        EXPECT_LE(posed[v].shows, 1.0f);
        EXPECT_EQ(posed[v].glow, 0.0f);
    }
}

TEST(RiggedHandTest, BendsWithTheFingers) {
    const RiggedHand& rig = RiggedHand::Get();
    std::vector<HandVertex> rest, posed;
    rig.Pose(rig.Rest(), rest);
    // The corner at the index finger's tip at rest.
    const Vec3 tip = rig.Rest().bones[1][2].End();
    size_t nearest = 0;
    for (size_t v = 1; v < rest.size(); ++v) {
        if (Length(Vec3{rest[v].x, rest[v].y, rest[v].z} - tip) <
            Length(Vec3{rest[nearest].x, rest[nearest].y, rest[nearest].z} - tip)) {
            nearest = v;
        }
    }
    DesignHand design = Relaxed();
    design.bends[1][0] = 0.8f;
    design.bends[1][1] = 1.1f;
    design.bends[1][2] = 0.7f;
    const HandSkeleton skeleton = hand_skeleton::Fit(design.Shown());
    // The index finger glowing.
    float glow[RiggedHand::kBones] = {};
    glow[4] = glow[5] = glow[6] = 1;
    rig.Pose(skeleton, posed, glow);
    // It stays at the bent finger's tip, and glows with it.
    EXPECT_LT(Length(Vec3{posed[nearest].x, posed[nearest].y, posed[nearest].z} -
                     skeleton.bones[1][2].End()),
              0.012f);
    EXPECT_NEAR(posed[nearest].glow, 1, 1e-4f);
    for (const HandVertex& v : posed) {
        EXPECT_LT(Length(Vec3{v.x, v.y, v.z} - skeleton.wrist), 0.25f);
        EXPECT_NEAR(Length(Vec3{v.nx, v.ny, v.nz}), 1, 1e-3f);
    }
}

TEST(RiggedHandTest, MirrorsTheRightHandForTheLeft) {
    const RiggedHand& rig = RiggedHand::Get();
    DesignHand right = Relaxed();
    DesignHand left = right;
    left.right = false;
    left.wrist.x = -right.wrist.x;
    left.along.x = -right.along.x;
    left.across.x = -right.across.x;
    std::vector<HandVertex> r, l;
    rig.Pose(hand_skeleton::Fit(right.Shown()), r);
    rig.Pose(hand_skeleton::Fit(left.Shown()), l);
    for (size_t v = 0; v < r.size(); ++v) {
        ExpectNear(Vec3{l[v].x, l[v].y, l[v].z}, Vec3{-r[v].x, r[v].y, r[v].z}, 1e-3f,
                   "mirrored corner");
        ExpectNear(Vec3{l[v].nx, l[v].ny, l[v].nz}, Vec3{-r[v].nx, r[v].ny, r[v].nz}, 2e-2f,
                   "mirrored normal");
    }
}

// A relaxed hand with its middle, ring and little fingers curled into the palm.
DesignHand Curled() {
    DesignHand hand = Relaxed();
    for (int f = 2; f < HandSkeleton::kFingers; ++f) {
        hand.bends[f][0] = 1.5f;
        hand.bends[f][1] = 1.8f;
        hand.bends[f][2] = 1.26f;
    }
    return hand;
}

// `hand` with its thumb's tip moved to `tip`, its last bone pointing there.
HandSkeleton ThumbAt(HandSkeleton hand, Vec3 tip) {
    Bone& last = hand.bones[0][HandSkeleton::kBones - 1];
    last.start = tip - last.axes.along * last.length;
    hand.tipsApart = -1;  // its own tips say
    return hand;
}

TEST(HandShapesTest, AnOpenHandMakesNoShape) {
    EXPECT_TRUE(hand_shapes::Read(hand_skeleton::Fit(Relaxed().Shown())).none());
}

TEST(HandShapesTest, TellsAPointFromAnL) {
    HandSkeleton hand = hand_skeleton::Fit(Curled().Shown());
    HandShapes shapes = hand_shapes::Read(hand);
    EXPECT_TRUE(shapes[size_t(HandShape::kL)]);
    EXPECT_FALSE(shapes[size_t(HandShape::kPoint)]);
    // The thumb on the middle finger's middle joint: pointing.
    shapes = hand_shapes::Read(ThumbAt(hand, hand.bones[2][1].start));
    EXPECT_TRUE(shapes[size_t(HandShape::kPoint)]);
}

TEST(HandShapesTest, PinchesAsTheTipsMeet) {
    const HandSkeleton hand = hand_skeleton::Fit(Relaxed().Shown());
    const Vec3 index = hand.bones[1][2].End(), away = hand.palm.across;
    const HandSkeleton apart = ThumbAt(hand, index + away * 0.05f);
    const HandSkeleton near = ThumbAt(hand, index + away * 0.035f);
    const HandSkeleton pinched = ThumbAt(hand, index + away * 0.02f);
    EXPECT_EQ(hand_shapes::Pinching(apart), 0.0f);
    EXPECT_NEAR(hand_shapes::Pinching(near), 0.5f, 1e-3f);
    EXPECT_EQ(hand_shapes::Pinching(pinched), 1.0f);
    EXPECT_FALSE(hand_shapes::Read(near)[size_t(HandShape::kPinch)]);
    EXPECT_TRUE(hand_shapes::Read(pinched)[size_t(HandShape::kPinch)]);
}

TEST(HandShapesTest, WatchesEachHandNowAndThen) {
    const HandSkeleton open = hand_skeleton::Fit(Relaxed().Shown());
    const HandSkeleton pinched = ThumbAt(open, open.bones[1][2].End());
    HandShapeWatcher watcher;
    EXPECT_TRUE(watcher.Update({open}, 1.0).empty());
    // Not checked again so soon.
    EXPECT_TRUE(watcher.Update({pinched}, 1.05).empty());
    std::vector<HandShapeChange> changes = watcher.Update({pinched}, 1.2);
    ASSERT_EQ(changes.size(), 1u);
    EXPECT_TRUE(changes[0].right);
    EXPECT_EQ(changes[0].shape, HandShape::kPinch);
    EXPECT_TRUE(changes[0].began);
    EXPECT_TRUE(watcher.Update({pinched}, 1.4).empty());
    // The hand gone, its pinch ends.
    changes = watcher.Update({}, 1.5);
    ASSERT_EQ(changes.size(), 1u);
    EXPECT_FALSE(changes[0].began);
}

// A gray picture `width` x `height`, `paint` setting each pixel's RGB.
template <typename Paint>
RgbaFrame Picture(int width, int height, Paint paint) {
    RgbaFrame frame;
    frame.width = width;
    frame.height = height;
    frame.rgba.resize(size_t(width) * height * 4);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            uint8_t* p = &frame.rgba[(size_t(y) * width + x) * 4];
            paint(x, y, p);
            p[3] = 255;
        }
    }
    return frame;
}

TEST(LightEstimateTest, SeesALightInView) {
    // A lamp up and to the right.
    RgbaFrame frame = Picture(160, 120, [](int x, int y, uint8_t* p) {
        bool lamp = std::hypot(x - 130, y - 20) < 8;
        p[0] = p[1] = p[2] = lamp ? 255 : 50;
    });
    SceneLight light = light_estimate::Estimate(frame, 0.6f, 0.45f);
    EXPECT_GT(light.from.x, 0.3f);
    EXPECT_LT(light.from.y, -0.1f);
    EXPECT_GT(light.from.z, 0);  // ahead, behind the hands
}

TEST(LightEstimateTest, TakesTheBrighterSideForALightOutOfView) {
    // Brighter at the top, as under a ceiling light, with nothing bright in view.
    RgbaFrame frame = Picture(160, 120, [](int, int y, uint8_t* p) {
        p[0] = p[1] = p[2] = uint8_t(160 - y);
    });
    SceneLight light = light_estimate::Estimate(frame, 0.6f, 0.45f);
    EXPECT_LT(light.from.y, -0.5f);
    EXPECT_LT(light.from.z, 0);  // from in front of the hands
    EXPECT_NEAR(light.from.x, 0, 0.05f);
}

TEST(LightEstimateTest, TakesTheLightsColor) {
    RgbaFrame warm = Picture(80, 60, [](int, int, uint8_t* p) {
        p[0] = 200;
        p[1] = 150;
        p[2] = 90;
    });
    SceneLight light = light_estimate::Estimate(warm, 0.6f, 0.45f);
    EXPECT_FLOAT_EQ(light.color[0], 1);
    EXPECT_GT(light.color[0], light.color[2]);
    EXPECT_GT(light.color[2], 0.5f);  // softened toward white
    // Halfway from the default light toward it.
    SceneLight half = light_estimate::Blend(SceneLight(), light, 0.5f);
    EXPECT_NEAR(half.color[2], (1 + light.color[2]) / 2, 1e-5f);
}

// The palm detector's outputs with nothing found, but where `found` says.
struct PalmOutputs {
    std::vector<float> boxes = std::vector<float>(hand_detection::kAnchors * hand_detection::kBoxValues);
    std::vector<float> scores = std::vector<float>(hand_detection::kAnchors, -10.0f);
    // A palm at anchor `anchor`, `score` sure (before its sigmoid), moved `dx`, `dy` and
    // `size` wide, in the detector's pixels; its keypoints on its middle.
    void Found(int anchor, float score, float dx, float dy, float size) {
        scores[anchor] = score;
        float* box = boxes.data() + anchor * hand_detection::kBoxValues;
        box[0] = dx;
        box[1] = dy;
        box[2] = box[3] = size;
        for (int k = 0; k < Palm::kKeypoints; ++k) {
            box[4 + 2 * k] = dx;
            box[5 + 2 * k] = dy;
        }
    }
};

TEST(HandDetectionTest, ReadsThePalmsFound) {
    using namespace hand_detection;
    // A picture twice as wide as high: the detector looks at a square as wide as it.
    const PictureSquare whole = WholePicture(384, 192);
    EXPECT_EQ(whole.size, 384);
    PalmOutputs outputs;
    // The first anchor is the first cell's middle, 4 of the detector's pixels in each way.
    outputs.Found(0, 4, 2, 0, 40);
    // The same palm, less sure, from the next anchor in that cell: blended in.
    outputs.Found(1, 2, 4, 0, 40);
    // Another, at the last anchor, the last cell's middle.
    outputs.Found(kAnchors - 1, 3, 0, 0, 20);
    const std::vector<Palm> palms = ReadPalms(outputs.boxes.data(), outputs.scores.data(), whole);
    ASSERT_EQ(palms.size(), 2u);
    const float sure = 1 / (1 + std::exp(-4.0f)), less = 1 / (1 + std::exp(-2.0f));
    EXPECT_NEAR(palms[0].score, sure, 1e-5f);
    // Each of the detector's pixels is two of the picture's, from 96 above its top.
    EXPECT_NEAR(palms[0].x, 2 * (4 + (2 * sure + 4 * less) / (sure + less)), 1e-3f);
    EXPECT_NEAR(palms[0].y, 2 * 4 - 96, 1e-3f);
    EXPECT_NEAR(palms[0].width, 80, 1e-3f);
    EXPECT_NEAR(palms[1].x, 2 * (192 - 8), 1e-3f);
    EXPECT_NEAR(palms[1].y, 2 * (192 - 8) - 96, 1e-3f);
    // Nothing sure enough: none.
    EXPECT_TRUE(ReadPalms(PalmOutputs().boxes.data(), PalmOutputs().scores.data(), whole).empty());
}

TEST(HandDetectionTest, TurnsTheSquareSoTheFingersPointUp) {
    Palm palm;
    palm.x = 200;
    palm.y = 100;
    palm.width = palm.height = 50;
    auto keypoints = [&](float wristX, float wristY, float knuckleX, float knuckleY) {
        palm.keypoints[0][0] = wristX;
        palm.keypoints[0][1] = wristY;
        palm.keypoints[2][0] = knuckleX;
        palm.keypoints[2][1] = knuckleY;
    };
    // Fingers up: not turned, and moved up toward them, wider than the palm.
    keypoints(200, 120, 200, 80);
    PictureSquare square = hand_detection::AroundPalm(palm);
    EXPECT_NEAR(square.angle, 0, 1e-5f);
    EXPECT_NEAR(square.x, 200, 1e-3f);
    EXPECT_NEAR(square.y, 75, 1e-3f);
    EXPECT_NEAR(square.size, 130, 1e-3f);
    // Fingers to the right: turned a quarter clockwise, and moved right.
    keypoints(180, 100, 220, 100);
    square = hand_detection::AroundPalm(palm);
    EXPECT_NEAR(square.angle, 1.5707964f, 1e-5f);
    EXPECT_NEAR(square.x, 225, 1e-3f);
    EXPECT_NEAR(square.y, 100, 1e-3f);
}

TEST(HandDetectionTest, ReadsTheHandFromItsSquare) {
    const PictureSquare square = {300, 200, 112, 0.5f};
    std::vector<float> landmarks(hand_detection::kLandmarkValues), shape(landmarks.size());
    // The wrist at the square's middle, the next landmark at its right edge's middle; the
    // shape's first point a unit right, the next a unit away.
    landmarks[0] = landmarks[1] = 112;
    landmarks[3] = 224;
    landmarks[4] = 112;
    shape[0] = 1;
    shape[5] = 1;
    const HandLandmarks hand =
        hand_detection::ReadHand(square, landmarks.data(), shape.data(), 0.9f, 600, 400);
    EXPECT_NEAR(hand.image[0][0], 0.5f, 1e-5f);
    EXPECT_NEAR(hand.image[0][1], 0.5f, 1e-5f);
    EXPECT_NEAR(hand.image[1][0] * 600, 300 + 56 * std::cos(0.5f), 1e-3f);
    EXPECT_NEAR(hand.image[1][1] * 400, 200 + 56 * std::sin(0.5f), 1e-3f);
    // The shape turned back as the square was.
    EXPECT_NEAR(hand.shape[0][0], std::cos(0.5f), 1e-5f);
    EXPECT_NEAR(hand.shape[0][1], std::sin(0.5f), 1e-5f);
    EXPECT_NEAR(hand.shape[1][2], 1, 1e-5f);
    EXPECT_TRUE(hand.right);
    EXPECT_FALSE(hand_detection::ReadHand(square, landmarks.data(), shape.data(), 0.1f, 600, 400)
                     .right);
}

TEST(HandDetectionTest, LooksForTheHandNextAroundItsLandmarks) {
    // Fingers pointing left, a little up.
    const HandLandmarks hand = Hand(0.6f, 0.5f, 0.1f, -1.3f);
    const PictureSquare square = hand_detection::AroundHand(hand, 1000, 1000);
    EXPECT_NEAR(square.angle, -1.3f, 0.05f);
    // Round the palm and the fingers' bases, with room for the fingers.
    for (int i : {0, 1, 5, 9, 13, 17}) {
        const float dx = hand.image[i][0] * 1000 - square.x, dy = hand.image[i][1] * 1000 - square.y;
        EXPECT_LT(std::hypot(dx, dy), square.size / 2) << "landmark " << i;
    }
    EXPECT_GT(square.size, 2 * 100);
    EXPECT_GT(hand_detection::Overlap(square, square), 0.999f);
    EXPECT_EQ(hand_detection::Overlap(square, {square.x + 2 * square.size, square.y, square.size, 0}),
              0);
}

// A palm of the detector's at (x, y) in the picture, its fingers up.
Palm PalmAt(float x, float y) {
    Palm palm;
    palm.score = 0.9f;
    palm.x = x;
    palm.y = y;
    palm.width = palm.height = 40;
    palm.keypoints[0][0] = palm.keypoints[2][0] = x;
    palm.keypoints[0][1] = y + 20;
    palm.keypoints[2][1] = y - 20;
    return palm;
}

TEST(HandTrackingTest, LooksForHandsOnlyTillItFollowsThem) {
    HandTracking tracking;
    EXPECT_TRUE(tracking.Searching(0));
    tracking.Search({}, 0);
    EXPECT_TRUE(tracking.Finish().empty());
    // Still none: it looks every picture.
    EXPECT_TRUE(tracking.Searching(0.03));
    tracking.Search({PalmAt(100, 300)}, 0.03);
    ASSERT_EQ(tracking.Squares().size(), 1u);
    // One: only now and then, for the other.
    EXPECT_FALSE(tracking.Searching(0.1));
    EXPECT_TRUE(tracking.Searching(0.03 + HandTracking::kSearchEvery));
    // The same hand again isn't another; a second one is.
    tracking.Search({PalmAt(102, 301), PalmAt(500, 300)}, 0.3);
    EXPECT_EQ(tracking.Squares().size(), 2u);
    EXPECT_FALSE(tracking.Searching(10));
}

TEST(HandTrackingTest, FollowsEachHandTillItsGone) {
    HandTracking tracking;
    tracking.Search({PalmAt(200, 300), PalmAt(500, 300)}, 0);
    ASSERT_EQ(tracking.Squares().size(), 2u);
    // The first hand there, moved right; the second gone.
    tracking.Answer(0, 0.95f, Hand(0.25f, 0.6f, 0.1f, 0), 1000, 1000);
    tracking.Answer(1, 0.2f, Hand(0.5f, 0.6f, 0.1f, 0), 1000, 1000);
    std::vector<HandLandmarks> hands = tracking.Finish();
    ASSERT_EQ(hands.size(), 1u);
    EXPECT_NEAR(hands[0].image[0][0], 0.25f, 1e-6f);
    ASSERT_EQ(tracking.Squares().size(), 1u);
    EXPECT_NEAR(tracking.Squares()[0].x, 250, 20);
    // Two squares that slid onto one hand: the surer keeps it.
    tracking.Search({PalmAt(700, 300)}, 1);
    ASSERT_EQ(tracking.Squares().size(), 2u);
    tracking.Answer(0, 0.8f, Hand(0.4f, 0.6f, 0.1f, 0), 1000, 1000);
    tracking.Answer(1, 0.9f, Hand(0.41f, 0.6f, 0.1f, 0), 1000, 1000);
    hands = tracking.Finish();
    ASSERT_EQ(hands.size(), 1u);
    EXPECT_NEAR(hands[0].image[0][0], 0.41f, 1e-6f);
    tracking.Clear();
    EXPECT_TRUE(tracking.Squares().empty());
    EXPECT_TRUE(tracking.Searching(1));
}

TEST(HandGeometryTest, TurnsAHandEvenWhenItsShapeDoesntFit) {
    // The shape the tracker gives, its landmarks 5 cm off either way: the picture can't show it.
    HandLandmarks hand = Relaxed().Tracked(0.6f);
    for (int i = 0; i < HandLandmarks::kCount; ++i) {
        hand.shape[i][0] += i % 2 ? 0.05f : -0.05f;
    }
    const ShownHand shown = hand_geometry::Show(hand, 0.6f, 0.6f);
    EXPECT_FALSE(shown.solid);
    // Its depths are still the shape's, at the distance the palm looks.
    const float wrist = hand_geometry::kPalmLength / shown.palm;
    for (int i = 1; i < HandLandmarks::kCount; ++i) {
        EXPECT_NEAR(shown.depth[i] * wrist, hand.shape[i][2] - hand.shape[0][2], 1e-4f);
    }
    EXPECT_GT(std::fabs(shown.depth[HandLandmarks::kLittleBase]), 0.01f);
}

// How far apart two skeletons' joints are, at most.
float Apart(const HandSkeleton& a, const HandSkeleton& b) {
    float most = Length(a.wrist - b.wrist);
    for (int f = 0; f < HandSkeleton::kFingers; ++f) {
        for (int b2 = 0; b2 < HandSkeleton::kBones; ++b2) {
            most = std::max(most, Length(a.bones[f][b2].End() - b.bones[f][b2].End()));
        }
    }
    return most;
}

void ExpectAHand(const HandSkeleton& hand) {
    for (int f = 0; f < HandSkeleton::kFingers; ++f) {
        for (int b = 0; b < HandSkeleton::kBones; ++b) {
            const Bone& bone = hand.bones[f][b];
            EXPECT_NEAR(bone.length, hand_skeleton::kLengths[f][b], 1e-6f);
            EXPECT_NEAR(Length(bone.axes.along), 1, 1e-4f);
            EXPECT_NEAR(Dot(bone.axes.along, bone.axes.palmward), 0, 1e-4f);
            if (b > 0) ExpectNear(bone.start, hand.bones[f][b - 1].End(), 1e-6f, "joint");
        }
    }
}

TEST(HandSkeletonTest, GlidesAlwaysAsAHand) {
    const HandSkeleton open = hand_skeleton::Fit(Relaxed().Shown());
    DesignHand fist = Curled();
    fist.wrist = fist.wrist + Vec3{0.05f, -0.03f, 0.02f};
    fist.across = {-1, 0, 0.8f};
    const HandSkeleton closed = hand_skeleton::Fit(fist.Shown());
    HandSkeleton drawn = open;
    float last = Apart(drawn, closed);
    for (int frame = 0; frame < 60; ++frame) {
        hand_skeleton::Glide(drawn, closed, 1 / 60.0f);
        ExpectAHand(drawn);
        const float now = Apart(drawn, closed);
        EXPECT_LT(now, last + 1e-6f);
        last = now;
    }
    EXPECT_LT(last, 1e-3f);
}

TEST(HandSkeletonTest, GlidesSlowlyThroughWobble) {
    // The same hand fitted a little turned (the tracker's wobble), and turned a lot (a move).
    DesignHand hand = Relaxed();
    const HandSkeleton from = hand_skeleton::Fit(hand.Shown());
    hand.across = {-1, 0, 0.25f};
    const HandSkeleton wobbled = hand_skeleton::Fit(hand.Shown());
    hand.across = {-1, 0, 1.2f};
    const HandSkeleton turned = hand_skeleton::Fit(hand.Shown());
    auto share = [&](const HandSkeleton& to) {
        HandSkeleton drawn = from;
        hand_skeleton::Glide(drawn, to, 1 / 60.0f);
        return 1 - Apart(drawn, to) / Apart(from, to);
    };
    EXPECT_LT(share(wobbled), 0.5f * share(turned));
}

TEST(HandFollowerTest, GlidesItsSkeletonsToEachAnswer) {
    HandFollower follower;
    follower.Update({Seen(0.5f, 0.9f, 0.2f, 0)}, kCameraTan, kCameraTan, 0);
    const HandSkeleton first = follower.Skeletons(0)[0];
    for (double now = 1 / 60.0; now < 0.1; now += 1 / 60.0) {
        follower.Skeletons(now);  // drawn every refresh
    }
    // A big move: drawn part of the way at once, then the rest as the screen refreshes.
    follower.Update({Seen(0.7f, 0.9f, 0.2f, 0)}, kCameraTan, kCameraTan, 0.1);
    const float moved = follower.Skeletons(0.1 + 1 / 60.0)[0].wrist.x - first.wrist.x;
    float last = moved;
    for (double now = 0.12; now < 0.38; now += 1 / 60.0) {
        last = follower.Skeletons(now)[0].wrist.x - first.wrist.x;
    }
    EXPECT_GT(moved, 0);
    EXPECT_LT(moved, 0.5f * last);
    EXPECT_EQ(follower.Skeletons(0.5).size(), 0u);  // lost
}

TEST(HandSkeletonTest, CountsTheTipsDepthLessForAPinch) {
    const DesignHand design = Relaxed();
    Vec3 at[HandLandmarks::kCount];
    design.Landmarks(at);
    // The tracker's thumb tip 4 cm behind its index tip, or 4 cm beside it.
    auto with = [&](Vec3 offset) {
        ShownHand shown = design.Shown();
        const Vec3 tip = at[8] + offset;
        shown.points[4] = {tip.x / tip.z, tip.y / tip.z};
        shown.depth[4] = tip.z / design.wrist.z - 1;
        return hand_skeleton::Fit(shown);
    };
    const Vec3 ray = Normalize(at[8], Vec3{0, 0, 1});
    const HandSkeleton behind = with(ray * 0.04f);
    const HandSkeleton beside = with(Normalize(Cross(ray, Vec3{0, 1, 0}), Vec3{1, 0, 0}) * 0.04f);
    EXPECT_NEAR(behind.tipsApart, 0.04f * 0.35f, 1e-3f);
    EXPECT_NEAR(beside.tipsApart, 0.04f, 1e-3f);
    EXPECT_TRUE(hand_shapes::Read(behind)[size_t(HandShape::kPinch)]);
    EXPECT_FALSE(hand_shapes::Read(beside)[size_t(HandShape::kPinch)]);
}

TEST(HandSkeletonTest, PinchesWhereTheTrackersTipsMeet) {
    const DesignHand design = Relaxed();
    ShownHand shown = design.Shown();
    const HandSkeleton open = hand_skeleton::Fit(shown);
    // The tracker's thumb tip a centimeter from its index tip.
    Vec3 at[HandLandmarks::kCount];
    design.Landmarks(at);
    const Vec3 tip = at[8] + Normalize(at[4] - at[8], Vec3{1, 0, 0}) * 0.01f;
    shown.points[4] = {tip.x / tip.z, tip.y / tip.z};
    shown.depth[4] = tip.z / design.wrist.z - 1;
    const HandSkeleton pinched = hand_skeleton::Fit(shown);
    auto apart = [](const HandSkeleton& hand) {
        return Length(hand.bones[0][2].End() - hand.bones[1][2].End());
    };
    EXPECT_GT(apart(open), 0.04f);
    EXPECT_NEAR(apart(pinched), 0.01f, 0.004f);
    EXPECT_EQ(hand_shapes::Pinching(pinched), 1.0f);
    // Its bones still their lengths.
    for (int f = 0; f < 2; ++f) {
        for (int b = 1; b < HandSkeleton::kBones; ++b) {
            ExpectNear(pinched.bones[f][b].start, pinched.bones[f][b - 1].End(), 1e-6f, "joint");
        }
    }
}

// A hand for the pointer, its wrist at (x, y) in the picture, fingers up; its thumb's tip on
// its index finger's when `pinched`.
ShownHand Pointing(float x, float y, bool pinched = false) {
    HandLandmarks hand = Hand(x, y, 0.15f, 0);
    hand.image[4][0] -= 0.08f;  // the thumb out to the side
    if (pinched) {
        hand.image[4][0] = hand.image[8][0] + 0.005f;
        hand.image[4][1] = hand.image[8][1];
    }
    return hand_geometry::Show(hand, kCameraTan, kCameraTan);
}

// Where that hand's palm points on the window.
void PalmOnWindow(const ShownHand& hand, float& x, float& y) {
    float u = 0, v = 0;
    for (int i : {0, 5, 9, 13, 17}) {
        u += (hand.points[i].x / kCameraTan + 1) / 2 / 5;
        v += (hand.points[i].y / kCameraTan + 1) / 2 / 5;
    }
    x = std::clamp((u - HandPointer::kLeft) / (HandPointer::kRight - HandPointer::kLeft), 0.0f, 1.0f);
    y = std::clamp((v - HandPointer::kTop) / (HandPointer::kBottom - HandPointer::kTop), 0.0f, 1.0f);
}

// The pointer after `seconds` of a hand held as `hand`, answered 10 times a second from `now`.
std::vector<PointerEvent> Hold(HandPointer& pointer, const ShownHand& hand, double& now,
                               double seconds) {
    std::vector<PointerEvent> events;
    for (double end = now + seconds; now < end; now += 0.1) {
        pointer.Update(&hand, kCameraTan, kCameraTan, now, events);
    }
    return events;
}

int Count(const std::vector<PointerEvent>& events, PointerAction action) {
    return int(std::count_if(events.begin(), events.end(),
                             [&](const PointerEvent& e) { return e.action == action; }));
}

TEST(HandPointerTest, PointsWithThePalmOverTheWholeWindow) {
    HandPointer pointer;
    double now = 0;
    const ShownHand middle = Pointing(0.45f, 0.6f);
    std::vector<PointerEvent> events = Hold(pointer, middle, now, 1);
    float x, y;
    PalmOnWindow(middle, x, y);
    ASSERT_FALSE(events.empty());
    EXPECT_EQ(Count(events, PointerAction::Move), int(events.size()));
    EXPECT_NEAR(events.back().x, x, 1e-3f);
    EXPECT_NEAR(events.back().y, y, 1e-3f);
    HandPointer::Cursor cursor = pointer.CursorAt(now);
    EXPECT_TRUE(cursor.shown);
    EXPECT_NEAR(cursor.x, x, 1e-2f);
    // Short of the camera's edge, the window's corner.
    events = Hold(pointer, Pointing(0.85f, 0.25f), now, 2);
    EXPECT_NEAR(events.back().x, 1, 1e-3f);
    EXPECT_NEAR(events.back().y, 0, 1e-3f);
    // No hand: no cursor.
    pointer.Update(nullptr, kCameraTan, kCameraTan, now, events);
    EXPECT_FALSE(pointer.CursorAt(now).shown);
}

TEST(HandPointerTest, SteadiesTheCursor) {
    HandPointer pointer;
    double now = 0;
    std::vector<PointerEvent> events = Hold(pointer, Pointing(0.5f, 0.6f), now, 1);
    const float settled = events.back().x;
    // The tracker's answers wobble about where the hand is.
    float most = 0;
    for (int k = 0; k < 20; ++k) {
        events.clear();
        pointer.Update(&static_cast<const ShownHand&>(Pointing(k % 2 ? 0.51f : 0.49f, 0.6f)),
                       kCameraTan, kCameraTan, now, events);
        now += 0.1;
        most = std::max(most, std::fabs(events.back().x - settled));
    }
    // Held still, the cursor stays still.
    EXPECT_LT(most, 1e-6f);
    // But a hand moving on moves it.
    events = Hold(pointer, Pointing(0.56f, 0.6f), now, 1);
    EXPECT_GT(events.back().x, settled + 0.1f);
}

TEST(HandPointerTest, ClicksWhereAPinchBegan) {
    HandPointer pointer;
    double now = 0;
    const ShownHand open = Pointing(0.5f, 0.6f);
    std::vector<PointerEvent> events = Hold(pointer, open, now, 1);
    const PointerEvent before = events.back();
    // Pinching nudges the hand a little; holding it clicks nothing yet.
    events = Hold(pointer, Pointing(0.505f, 0.605f, true), now, 0.5);
    EXPECT_TRUE(events.empty());
    EXPECT_TRUE(pointer.CursorAt(now).pinched);
    events = Hold(pointer, open, now, 0.2);
    ASSERT_EQ(Count(events, PointerAction::Click), 1);
    const PointerEvent& click = events.front();
    EXPECT_EQ(click.action, PointerAction::Click);
    EXPECT_NEAR(click.x, before.x, 2e-3f);
    EXPECT_NEAR(click.y, before.y, 2e-3f);
    // A pinch the hand is lost in doesn't click.
    Hold(pointer, Pointing(0.5f, 0.6f, true), now, 0.3);
    events.clear();
    pointer.Update(nullptr, kCameraTan, kCameraTan, now, events);
    events = Hold(pointer, open, now, 0.5);
    EXPECT_EQ(Count(events, PointerAction::Click), 0);
}

TEST(HandPointerTest, ScrollsWhenAPinchMoves) {
    HandPointer pointer;
    double now = 0;
    Hold(pointer, Pointing(0.5f, 0.6f), now, 1);
    Hold(pointer, Pointing(0.5f, 0.6f, true), now, 0.3);
    // Pinched, the hand moves up a fifth of the window: the content follows it up.
    std::vector<PointerEvent> events = Hold(pointer, Pointing(0.5f, 0.5f, true), now, 2);
    int vertical = 0;
    for (const PointerEvent& e : events) {
        if (e.action == PointerAction::Scroll) vertical += e.wheelVertical;
    }
    EXPECT_LT(vertical, 0);
    EXPECT_EQ(vertical % 120, 0);
    EXPECT_EQ(Count(events, PointerAction::Click), 0);
    // Letting go after scrolling doesn't click.
    events = Hold(pointer, Pointing(0.5f, 0.5f), now, 0.3);
    EXPECT_EQ(Count(events, PointerAction::Click), 0);
}

TEST(HandTrackingTest, SeesOneHandOnce) {
    HandTracking tracking;
    tracking.Search({PalmAt(200, 300)}, 0);
    ASSERT_EQ(tracking.Squares().size(), 1u);
    // The same palm found again, its box a different shape: still one hand.
    Palm again = PalmAt(212, 296);
    again.width = 70;
    again.height = 30;
    tracking.Search({again}, 0.3);
    EXPECT_EQ(tracking.Squares().size(), 1u);
}

TEST(HandFollowerTest, FollowsAHandSeenTwiceOnce) {
    HandFollower follower;
    follower.Update({Seen(0.5f, 0.9f, 0.2f, 0)}, kCameraTan, kCameraTan, 0);
    // The tracker sees it twice, nearly in one place: not a second hand.
    follower.Update({Seen(0.5f, 0.9f, 0.2f, 0), Seen(0.52f, 0.9f, 0.2f, 0.1f)}, kCameraTan,
                    kCameraTan, 0.1);
    EXPECT_EQ(follower.Hands(0.1).size(), 1u);
    // The viewer's other hand, beside it, is.
    follower.Update({Seen(0.5f, 0.9f, 0.2f, 0), Seen(0.75f, 0.9f, 0.2f, 0)}, kCameraTan,
                    kCameraTan, 0.2);
    EXPECT_EQ(follower.Hands(0.2).size(), 2u);
}

}  // namespace
}  // namespace gazeora
