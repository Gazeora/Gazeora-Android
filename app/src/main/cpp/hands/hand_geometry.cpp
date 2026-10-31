#include "hands/hand_geometry.h"

#include <algorithm>
#include <cmath>

namespace gazeora {

namespace hand_geometry {

namespace {

// Where a hand's 3D shape can be, in meters from the camera: nearer or farther, the shape and
// the picture don't agree.
constexpr float kNearestHand = 0.08f;
constexpr float kFarthestHand = 3.0f;
// How far off, on average, the shape put in place may show from the picture's landmarks, in
// the palm's apparent lengths, for them to agree.
constexpr float kWorstFit = 0.25f;

float Distance(const ViewTangent& a, const ViewTangent& b) {
    return std::hypot(a.x - b.x, a.y - b.y);
}

// Where the hand's middle is in front of the camera, in meters (x right, y down, z ahead), so
// that each landmark of its shape lines up with where the picture shows it: the least squares
// fit of middle.x - t.x middle.z = t.x shape.z - shape.x, and the same down. False if there's
// no such place.
bool PlaceShape(const HandLandmarks& hand, const ViewTangent (&points)[HandLandmarks::kCount],
                float (&middle)[3]) {
    // The normal equations, summed over both rows of every landmark.
    double n = 0, sx = 0, sy = 0, sxy2 = 0, bx = 0, by = 0, bz = 0;
    for (int i = 0; i < HandLandmarks::kCount; ++i) {
        const double x = points[i].x, y = points[i].y;
        const float* s = hand.shape[i];
        const double across = x * s[2] - s[0], down = y * s[2] - s[1];
        n += 1;
        sx += x;
        sy += y;
        sxy2 += x * x + y * y;
        bx += across;
        by += down;
        bz -= x * across + y * down;
    }
    // [n 0 -sx; 0 n -sy; -sx -sy sxy2] middle = [bx; by; bz]: z first, the rest from it.
    const double det = n * (n * sxy2 - sy * sy) - sx * sx * n;
    if (std::fabs(det) < 1e-12) {
        return false;
    }
    const double z = (n * n * bz + n * sx * bx + n * sy * by) / det;
    middle[0] = float((bx + sx * z) / n);
    middle[1] = float((by + sy * z) / n);
    middle[2] = float(z);
    return true;
}

}  // namespace

ShownHand Show(const HandLandmarks& hand, float tanAcross, float tanDown) {
    ShownHand shown;
    for (int i = 0; i < HandLandmarks::kCount; ++i) {
        shown.points[i].x = (2 * hand.image[i][0] - 1) * tanAcross;
        shown.points[i].y = (2 * hand.image[i][1] - 1) * tanDown;
    }
    shown.right = hand.right;
    const ViewTangent* p = shown.points;
    // How big the palm looks: its length, or from its width when it's seen end on.
    const float looks = std::max(
        Distance(p[HandLandmarks::kWrist], p[HandLandmarks::kMiddleBase]),
        Distance(p[HandLandmarks::kIndexBase], p[HandLandmarks::kLittleBase]) * kPalmLength /
            kPalmWidth);
    shown.palm = looks;
    // Where the shape and the picture don't agree, the shape's own depths still turn the hand,
    // from as far as the palm looks: never flat, which would show only its front or back.
    auto guess = [&]() -> ShownHand& {
        if (looks > 0) {
            const float wrist = kPalmLength / looks;
            const float* from = hand.shape[HandLandmarks::kWrist];
            for (int i = 0; i < HandLandmarks::kCount; ++i) {
                shown.depth[i] = (hand.shape[i][2] - from[2]) / wrist;
            }
        }
        return shown;
    };

    float middle[3];
    if (!PlaceShape(hand, shown.points, middle)) {
        return guess();
    }
    float z[HandLandmarks::kCount];
    float misfit = 0;
    for (int i = 0; i < HandLandmarks::kCount; ++i) {
        const float* s = hand.shape[i];
        z[i] = middle[2] + s[2];
        if (z[i] < kNearestHand || z[i] > kFarthestHand) {
            return guess();
        }
        float dx = (middle[0] + s[0]) / z[i] - p[i].x, dy = (middle[1] + s[1]) / z[i] - p[i].y;
        misfit += dx * dx + dy * dy;
    }
    if (std::sqrt(misfit / HandLandmarks::kCount) > kWorstFit * looks) {
        return guess();
    }
    const float wrist = z[HandLandmarks::kWrist];
    for (int i = 0; i < HandLandmarks::kCount; ++i) {
        shown.depth[i] = z[i] / wrist - 1;
    }
    shown.palm = kPalmLength / wrist;
    shown.solid = true;
    return shown;
}

float Rightness(const ShownHand& hand) {
    // In the camera's space, in wrist distances.
    float at[HandLandmarks::kCount][3];
    for (int i = 0; i < HandLandmarks::kCount; ++i) {
        float z = 1 + hand.depth[i];
        at[i][0] = hand.points[i].x * z;
        at[i][1] = hand.points[i].y * z;
        at[i][2] = z;
    }
    auto from = [&](int i, int origin, float (&out)[3]) {
        for (int k = 0; k < 3; ++k) out[k] = at[i][k] - at[origin][k];
    };
    auto dot = [](const float (&a)[3], const float (&b)[3]) {
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    };
    // The palm's plane: across from the little finger's knuckle to the index's, and along from
    // the wrist to between them. A right hand's palm side is across x along.
    float across[3], along[3], knuckles[3];
    from(HandLandmarks::kIndexBase, HandLandmarks::kLittleBase, across);
    for (int k = 0; k < 3; ++k) {
        knuckles[k] = (at[HandLandmarks::kIndexBase][k] + at[HandLandmarks::kLittleBase][k]) / 2 -
                      at[HandLandmarks::kWrist][k];
    }
    const float acrossLength = std::sqrt(dot(across, across));
    if (acrossLength <= 0) return 0;
    for (float& v : across) v /= acrossLength;
    const float onAcross = dot(knuckles, across);
    for (int k = 0; k < 3; ++k) along[k] = knuckles[k] - across[k] * onAcross;
    const float palmLength = std::sqrt(dot(along, along));
    if (palmLength <= 0) return 0;
    for (float& v : along) v /= palmLength;
    const float palmward[3] = {across[1] * along[2] - across[2] * along[1],
                               across[2] * along[0] - across[0] * along[2],
                               across[0] * along[1] - across[1] * along[0]};
    // The thumb's joints, and the fingertips, as far as they bend palmward.
    constexpr int kThumb[] = {2, 3, 4};
    constexpr int kTips[] = {8, 12, 16, 20};
    float rightness = 0, off[3];
    for (int i : kThumb) {
        from(i, HandLandmarks::kWrist, off);
        rightness += dot(off, palmward) / palmLength / 3;
    }
    for (int i : kTips) {
        from(i, HandLandmarks::kWrist, off);
        float bent = dot(off, palmward) / palmLength;
        rightness += 0.25f * bent / 4;
    }
    return rightness;
}

bool WithinReach(const ShownHand& hand) { return hand.palm >= kPalmLength / kArmsReach; }

bool ComesFromViewer(const ShownHand& hand, float tanAcross, float tanDown) {
    const ViewTangent& wrist = hand.points[HandLandmarks::kWrist];
    const ViewTangent& knuckle = hand.points[HandLandmarks::kMiddleBase];
    float length = Distance(wrist, knuckle);
    if (length <= 0 || knuckle.y - wrist.y > kMostDown * length) {
        return false;
    }
    // The elbow, on along the line from the knuckle through the wrist, as far as a forearm is
    // for a palm this size.
    float reach = hand.palm * kForearm / kPalmLength / length;
    float elbowX = wrist.x + (wrist.x - knuckle.x) * reach;
    float elbowY = wrist.y + (wrist.y - knuckle.y) * reach;
    return elbowY > tanDown || std::fabs(elbowX) > tanAcross;
}

}  // namespace hand_geometry

}  // namespace gazeora
