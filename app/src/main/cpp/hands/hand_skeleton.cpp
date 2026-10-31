#include "hands/hand_skeleton.h"

#include <algorithm>
#include <cmath>

namespace gazeora {

namespace hand_skeleton {

namespace {

// The tracker's landmarks along each finger, from its knuckle (the thumb's base) out.
constexpr int kLandmarks[HandSkeleton::kFingers][4] = {
    {1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}, {13, 14, 15, 16}, {17, 18, 19, 20}};
// How far each finger spreads from straight along the palm at rest, toward the thumb's side.
constexpr float kRestSpread[HandSkeleton::kFingers] = {0, 0.12f, 0.03f, -0.06f, -0.18f};
// A finger bent toward the palm, seen from it, has less telling how it spreads: how it's seen
// to spread counts not at all with less than the first share of its first bone's length lying
// along the palm, and fully from the second; between, its rest's counts for the rest.
constexpr float kSpreadUnseen = 0.25f;
constexpr float kSpreadSeen = 0.55f;
// A finger's tip mostly bends with its middle joint, by this share of it; the tracker's own
// say counts for this much.
constexpr float kTipFollows = 0.7f;
constexpr float kTipSeen = 0.6f;
// In a pinch, the thumb's and index finger's tips meet as the tracker's do: from this far
// apart, all the way at this, in meters, where they're drawn as far apart as the tracker's
// are. Their angles move only as far as that takes, each costing as much to move a radian as
// the tip this far off, in meters; the thumb's first bone turns this far at most from where
// the tracker saw it, in radians.
constexpr float kMeetFrom = 0.03f;
constexpr float kMeetAt = 0.015f;
constexpr float kStayNear = 0.012f;
// How much the tracker's tips being apart in depth counts toward how far apart they are: it
// sees where they are across the picture well, but how far away much less well, and a little
// error there would hide a pinch, or make one of a hand held open.
constexpr float kDepthCounts = 0.35f;
constexpr float kThumbReachTurn = 0.3f;
// They move in this many steps, each turning no angle more than this, in radians, and halved
// this many times at most till it gets nearer.
constexpr int kReachSteps = 6;
constexpr float kLongestStep = 0.2f;
constexpr int kStepTries = 4;
constexpr float kNumericStep = 1e-3f;
// The skeleton drawn glides to the newest fitted, smoothing the tracker's wobble away while
// following its moves: over this long, in seconds, for a part that's only this far off (in
// radians, or meters for the wrist), and this long for one this far off, which is moving.
constexpr float kGlideStill = 0.12f;
constexpr float kGlideMoving = 0.035f;
constexpr float kTurnStill = 0.05f;
constexpr float kTurnMoving = 0.35f;
constexpr float kShiftStill = 0.004f;
constexpr float kShiftMoving = 0.03f;

// A finger's angles: for a finger, how far it spreads and bends at each joint; for the thumb,
// how far its first bone turns from where the tracker saw it, across and palmward, then how
// far its next two bend.
struct Angles {
    static constexpr int kCount = 4;
    float a[kCount] = {};
};

// `axes` spread `angle` toward the thumb's side, about the palmward axis.
Axes Spread(const Axes& axes, float angle) {
    float c = std::cos(angle), s = std::sin(angle);
    return {axes.along * c + axes.across * s, axes.palmward, axes.across * c - axes.along * s};
}

// `axes` bent `angle` toward the palm, about the across axis.
Axes Bend(const Axes& axes, float angle) {
    float c = std::cos(angle), s = std::sin(angle);
    return {axes.along * c + axes.palmward * s, axes.palmward * c - axes.along * s, axes.across};
}

// How far `toward` is bent from `axes`'s along, toward its palmward side.
float BendTo(const Axes& axes, Vec3 toward) {
    return std::atan2(Dot(toward, axes.palmward), Dot(toward, axes.along));
}

Bone MakeBone(Vec3 start, const Axes& axes, int finger, int bone) {
    return {start, axes, kLengths[finger][bone]};
}

// Where finger `f`'s knuckle (the thumb's base) is on `skeleton`'s palm.
Vec3 Knuckle(const HandSkeleton& skeleton, int f) {
    const Axes& palm = skeleton.palm;
    return skeleton.wrist + palm.along * kKnuckles[f][0] + palm.across * kKnuckles[f][1] +
           palm.palmward * kKnuckles[f][2];
}

// How far a finger's angles go.
void Limits(int f, Angles& least, Angles& most) {
    if (f == 0) {
        least = {{-kThumbReachTurn, -kThumbReachTurn, kThumbMiddleBack, kThumbTipBack}};
        most = {{kThumbReachTurn, kThumbReachTurn, kThumbMiddleBend, kThumbTipBend}};
    } else {
        least = {{kRestSpread[f] - kSpread, kKnuckleBack, 0, 0}};
        most = {{kRestSpread[f] + kSpread, kKnuckleBend, kMiddleBend, kTipBend}};
    }
}

// A finger's bones from its angles. The thumb's first bone turns from `seen`, the way the
// tracker saw it, but no farther from its rest than a thumb turns.
void Build(const HandSkeleton& skeleton, int f, Vec3 seen, const Angles& angles,
           Bone (&bones)[HandSkeleton::kBones]) {
    const Axes& palm = skeleton.palm;
    const float* a = angles.a;
    Axes axes;
    if (f == 0) {
        const Axes turning = ThumbAxes(palm, seen);
        Vec3 first = Normalize(seen + turning.across * a[0] + turning.palmward * a[1], seen);
        const Vec3 rest = Normalize(palm.along * kThumbRest[0] + palm.across * kThumbRest[1] +
                                        palm.palmward * kThumbRest[2],
                                    palm.along);
        if (std::acos(std::clamp(Dot(first, rest), -1.0f, 1.0f)) > kThumbTurn) {
            Vec3 away = Normalize(Flatten(first, rest), palm.palmward);
            first = rest * std::cos(kThumbTurn) + away * std::sin(kThumbTurn);
        }
        axes = ThumbAxes(palm, first);
    } else {
        axes = Bend(Spread(palm, a[0]), a[1]);
    }
    bones[0] = MakeBone(Knuckle(skeleton, f), axes, f, 0);
    for (int b = 1; b < HandSkeleton::kBones; ++b) {
        axes = Bend(axes, a[1 + b]);  // the last two angles bend the joints after the first
        bones[b] = MakeBone(bones[b - 1].End(), axes, f, b);
    }
}

// How far off a finger's tip is, `off`, and its angles from `near`, each radian costing `stay`
// square meters.
float Cost(Vec3 off, const Angles& angles, const Angles& near, float stay) {
    float cost = Dot(off, off);
    for (int k = 0; k < Angles::kCount; ++k) {
        const float d = angles.a[k] - near.a[k];
        cost += stay * d * d;
    }
    return cost;
}

// The angles that bring finger `f`'s tip nearest `target`, starting from `angles` and staying
// near `near`: damped least squares (Gauss-Newton), in steps that only ever get nearer,
// within the finger's limits.
void Reach(const HandSkeleton& skeleton, int f, Vec3 seen, Vec3 target, const Angles& near,
           Angles& angles) {
    constexpr int n = Angles::kCount;
    Angles least, most;
    Limits(f, least, most);
    Bone bones[HandSkeleton::kBones];
    auto tip = [&](const Angles& at) {
        Build(skeleton, f, seen, at, bones);
        return bones[HandSkeleton::kBones - 1].End();
    };
    const float stay = kStayNear * kStayNear;
    for (int step = 0; step < kReachSteps; ++step) {
        const Vec3 now = tip(angles);
        const Vec3 off = target - now;
        Vec3 slope[n];
        for (int k = 0; k < n; ++k) {
            Angles nudged = angles;
            nudged.a[k] += kNumericStep;
            slope[k] = (tip(nudged) - now) * (1 / kNumericStep);
        }
        // (J'J + stay) d = J'off - stay (angles - near), solved by elimination.
        float m[n][n + 1];
        for (int r = 0; r < n; ++r) {
            for (int c = 0; c < n; ++c) {
                m[r][c] = Dot(slope[r], slope[c]) + (r == c ? stay : 0);
            }
            m[r][n] = Dot(slope[r], off) - stay * (angles.a[r] - near.a[r]);
        }
        for (int c = 0; c < n; ++c) {
            for (int r = c + 1; r < n; ++r) {
                const float factor = m[r][c] / m[c][c];
                for (int k = c; k <= n; ++k) m[r][k] -= factor * m[c][k];
            }
        }
        float d[n];
        for (int r = n - 1; r >= 0; --r) {
            float sum = m[r][n];
            for (int k = r + 1; k < n; ++k) sum -= m[r][k] * d[k];
            d[r] = sum / m[r][r];
        }
        // A step no longer than a hand moves that far, taken only if it gets nearer; else half
        // as long.
        const float longest = *std::max_element(d, d + n, [](float a, float b) {
            return std::fabs(a) < std::fabs(b);
        });
        float scale = std::min(1.0f, kLongestStep / std::max(std::fabs(longest), 1e-9f));
        const float cost = Cost(off, angles, near, stay);
        for (int tries = 0; tries < kStepTries; ++tries, scale /= 2) {
            Angles moved = angles;
            for (int k = 0; k < n; ++k) {
                moved.a[k] = std::clamp(angles.a[k] + d[k] * scale, least.a[k], most.a[k]);
            }
            if (Cost(target - tip(moved), moved, near, stay) < cost) {
                angles = moved;
                break;
            }
        }
    }
}

// Finger `f`'s angles as near as a hand bends to how the tracker saw it, `at`.
Angles FitFinger(const Vec3 (&at)[HandLandmarks::kCount], const HandSkeleton& skeleton, int f) {
    const Axes& palm = skeleton.palm;
    const int* landmark = kLandmarks[f];
    Angles angles;
    float* a = angles.a;
    // The knuckle: how far the first bone spreads, and how far it bends.
    const Vec3 first = at[landmark[1]] - at[landmark[0]];
    const float along = Dot(first, palm.along), across = Dot(first, palm.across);
    const float flat = std::hypot(along, across);
    const float seenSpread = std::clamp(std::atan2(across, along), kRestSpread[f] - kSpread,
                                        kRestSpread[f] + kSpread);
    const float seen = std::clamp(
        (flat / std::max(Length(first), 1e-6f) - kSpreadUnseen) / (kSpreadSeen - kSpreadUnseen),
        0.0f, 1.0f);
    a[0] = kRestSpread[f] + (seenSpread - kRestSpread[f]) * seen * seen * (3 - 2 * seen);
    a[1] = std::clamp(std::atan2(Dot(first, palm.palmward), flat), kKnuckleBack, kKnuckleBend);
    // The middle joint, and the tip's, which only bend toward the palm.
    const Axes firstAxes = Bend(Spread(palm, a[0]), a[1]);
    a[2] = std::clamp(BendTo(firstAxes, at[landmark[2]] - at[landmark[1]]), 0.0f, kMiddleBend);
    const Axes middleAxes = Bend(firstAxes, a[2]);
    const float tip = BendTo(middleAxes, at[landmark[3]] - at[landmark[2]]);
    a[3] = std::clamp(kTipSeen * tip + (1 - kTipSeen) * kTipFollows * a[2], 0.0f, kTipBend);
    return angles;
}

// The thumb's angles, the same way, from its first bone `seen` as the tracker saw it.
Angles FitThumb(const Vec3 (&at)[HandLandmarks::kCount], const HandSkeleton& skeleton,
                Vec3 seen) {
    const int* landmark = kLandmarks[0];
    Angles angles;
    Bone bones[HandSkeleton::kBones];
    Build(skeleton, 0, seen, angles, bones);
    const Axes& axes = bones[0].axes;
    angles.a[2] = std::clamp(BendTo(axes, at[landmark[2]] - at[landmark[1]]), kThumbMiddleBack,
                             kThumbMiddleBend);
    const Axes middleAxes = Bend(axes, angles.a[2]);
    angles.a[3] = std::clamp(BendTo(middleAxes, at[landmark[3]] - at[landmark[2]]),
                             kThumbTipBack, kThumbTipBend);
    return angles;
}

// 1 for axes as a right hand's are, -1 for a left's, mirrored.
float Handed(const Axes& axes) {
    return Dot(Cross(axes.across, axes.along), axes.palmward) < 0 ? -1.0f : 1.0f;
}

// A rotation as a unit quaternion.
struct Turn {
    float w = 1, x = 0, y = 0, z = 0;
};

// `axes`, made a proper rotation by flipping palmward where `handed` is -1 (a left hand's).
Turn ToTurn(const Axes& axes, float handed) {
    const Vec3 a = axes.along, p = axes.palmward * handed, c = axes.across;
    // The matrix with columns along, palmward, across.
    const float m00 = a.x, m01 = p.x, m02 = c.x;
    const float m10 = a.y, m11 = p.y, m12 = c.y;
    const float m20 = a.z, m21 = p.z, m22 = c.z;
    Turn t;
    const float trace = m00 + m11 + m22;
    if (trace > 0) {
        const float s = 2 * std::sqrt(trace + 1);
        t = {s / 4, (m21 - m12) / s, (m02 - m20) / s, (m10 - m01) / s};
    } else if (m00 > m11 && m00 > m22) {
        const float s = 2 * std::sqrt(1 + m00 - m11 - m22);
        t = {(m21 - m12) / s, s / 4, (m01 + m10) / s, (m02 + m20) / s};
    } else if (m11 > m22) {
        const float s = 2 * std::sqrt(1 + m11 - m00 - m22);
        t = {(m02 - m20) / s, (m01 + m10) / s, s / 4, (m12 + m21) / s};
    } else {
        const float s = 2 * std::sqrt(1 + m22 - m00 - m11);
        t = {(m10 - m01) / s, (m02 + m20) / s, (m12 + m21) / s, s / 4};
    }
    return t;
}

Axes ToAxes(const Turn& t, float handed) {
    const float w = t.w, x = t.x, y = t.y, z = t.z;
    const Vec3 along = {1 - 2 * (y * y + z * z), 2 * (x * y + w * z), 2 * (x * z - w * y)};
    const Vec3 palmward = {2 * (x * y - w * z), 1 - 2 * (x * x + z * z), 2 * (y * z + w * x)};
    const Vec3 across = {2 * (x * z + w * y), 2 * (y * z - w * x), 1 - 2 * (x * x + y * y)};
    return {along, palmward * handed, across};
}

// How long a part glides for, being `off` from where it's going: kGlideStill at `still` or
// less, kGlideMoving at `moving` or more.
float GlideTime(float off, float still, float moving) {
    const float x = std::clamp((off - still) / (moving - still), 0.0f, 1.0f);
    return kGlideStill + (kGlideMoving - kGlideStill) * x * x * (3 - 2 * x);
}

// `axes` turned toward `toward` for `seconds`.
void GlideAxes(Axes& axes, const Axes& toward, float seconds) {
    const float handed = Handed(toward);
    const Turn from = ToTurn(axes, handed);
    Turn to = ToTurn(toward, handed);
    float dot = from.w * to.w + from.x * to.x + from.y * to.y + from.z * to.z;
    if (dot < 0) {
        to = {-to.w, -to.x, -to.y, -to.z};
        dot = -dot;
    }
    const float off = 2 * std::acos(std::min(dot, 1.0f));
    const float share = 1 - std::exp(-seconds / GlideTime(off, kTurnStill, kTurnMoving));
    Turn t = {from.w + (to.w - from.w) * share, from.x + (to.x - from.x) * share,
              from.y + (to.y - from.y) * share, from.z + (to.z - from.z) * share};
    const float length = std::sqrt(t.w * t.w + t.x * t.x + t.y * t.y + t.z * t.z);
    t = {t.w / length, t.x / length, t.y / length, t.z / length};
    axes = ToAxes(t, handed);
}

}  // namespace

Axes FingerAxes(const Axes& palm, Vec3 along) {
    // Spread and bent from the palm's, its across still lies in the palm.
    const float handed = Handed(palm);
    Axes axes;
    axes.along = along;
    axes.across = Normalize(Cross(along, palm.palmward), palm.across) * handed;
    axes.palmward = Cross(axes.across, along) * handed;
    return axes;
}

Axes ThumbAxes(const Axes& palm, Vec3 along) {
    // It curls across the palm, toward the little finger.
    const Vec3 curl =
        palm.along * kThumbCurl[0] + palm.across * kThumbCurl[1] + palm.palmward * kThumbCurl[2];
    Axes axes;
    axes.along = along;
    axes.palmward = Normalize(Flatten(curl, along), Normalize(Flatten(palm.palmward, along),
                                                              palm.across));
    axes.across = Cross(axes.palmward, axes.along);
    return axes;
}

Axes NextAxes(const Axes& before, Vec3 along) {
    // Bent about its across, which it keeps.
    const float handed = Handed(before);
    Axes axes;
    axes.along = along;
    axes.across = Normalize(Flatten(before.across, along), before.across);
    axes.palmward = Cross(axes.across, along) * handed;
    return axes;
}

HandSkeleton Fit(const ShownHand& hand) {
    HandSkeleton skeleton;
    skeleton.right = hand.right;
    // The landmarks in meters: the wrist is as far as a palm that size is.
    const float wristDistance = hand_geometry::kPalmLength / std::max(hand.palm, 1e-6f);
    Vec3 at[HandLandmarks::kCount];
    for (int i = 0; i < HandLandmarks::kCount; ++i) {
        float z = 1 + hand.depth[i];
        at[i] = Vec3{hand.points[i].x * z, hand.points[i].y * z, z} * wristDistance;
    }
    skeleton.wrist = at[HandLandmarks::kWrist];
    // The palm's axes, from the line between the little and index knuckles and the line from
    // the wrist to the middle one, which lie in it as they do in the skeleton's palm (the
    // little knuckle a little nearer the wrist): each line is so much along and so much
    // across, so along and across are the two lines mixed back apart. Palmward is square to
    // both, one way for a right hand and the other for a left.
    const float* index = kKnuckles[1];
    const float* middle = kKnuckles[2];
    const float* little = kKnuckles[4];
    const float knucklesAlong = index[0] - little[0], knucklesAcross = index[1] - little[1];
    const float middleAlong = middle[0], middleAcross = middle[1];
    const float det = knucklesAlong * middleAcross - knucklesAcross * middleAlong;
    const Vec3 knuckles = at[HandLandmarks::kIndexBase] - at[HandLandmarks::kLittleBase];
    const Vec3 toMiddle = at[HandLandmarks::kMiddleBase] - skeleton.wrist;
    const Vec3 along = (knuckles * middleAcross - toMiddle * knucklesAcross) * (1 / det);
    const Vec3 across = (toMiddle * knucklesAlong - knuckles * middleAlong) * (1 / det);
    Axes& palm = skeleton.palm;
    palm.along = Normalize(along, Vec3{0, -1, 0});
    palm.across = Normalize(Flatten(across, palm.along),
                            Normalize(Cross(palm.along, Vec3{0, 0, -1}), Vec3{1, 0, 0}));
    palm.palmward = Cross(palm.across, palm.along) * (hand.right ? 1.0f : -1.0f);

    // Each finger bent as near as a hand can to how the tracker saw it. (The thumb's first bone
    // turns from the way the tracker saw it.)
    const Vec3 thumb = Normalize(at[kLandmarks[0][1]] - at[kLandmarks[0][0]], palm.along);
    Angles angles[HandSkeleton::kFingers];
    for (int f = 0; f < HandSkeleton::kFingers; ++f) {
        angles[f] = f == 0 ? FitThumb(at, skeleton, thumb) : FitFinger(at, skeleton, f);
        Build(skeleton, f, thumb, angles[f], skeleton.bones[f]);
    }
    // In a pinch, the tracker's thumb and index tips meet, and so do the skeleton's: the thumb
    // as far toward their middle as it goes, then the index finger, which bends more freely,
    // the rest of the way to it.
    const Vec3 thumbSeen = at[kLandmarks[0][3]], indexSeen = at[kLandmarks[1][3]];
    const Vec3 ray = Normalize(thumbSeen + indexSeen, Vec3{0, 0, 1});
    const Vec3 between = thumbSeen - indexSeen;
    const float deep = Dot(between, ray);
    const float apart = Length(Flatten(between, ray) + ray * (deep * kDepthCounts));
    skeleton.tipsApart = apart;
    const float meeting = std::clamp((kMeetFrom - apart) / (kMeetFrom - kMeetAt), 0.0f, 1.0f);
    if (meeting > 0) {
        const Vec3 thumbTip = skeleton.bones[0][HandSkeleton::kBones - 1].End();
        const Vec3 indexTip = skeleton.bones[1][HandSkeleton::kBones - 1].End();
        const Vec3 middle = (thumbTip + indexTip) * 0.5f;
        const Vec3 way = Normalize(thumbTip - indexTip, palm.across);
        const float share = meeting * meeting * (3 - 2 * meeting);
        const Angles thumbFrom = angles[0], indexFrom = angles[1];
        Reach(skeleton, 0, thumb, thumbTip + (middle + way * (apart / 2) - thumbTip) * share,
              thumbFrom, angles[0]);
        Build(skeleton, 0, thumb, angles[0], skeleton.bones[0]);
        const Vec3 thumbMet = skeleton.bones[0][HandSkeleton::kBones - 1].End();
        Reach(skeleton, 1, thumb, indexTip + (thumbMet - way * apart - indexTip) * share,
              indexFrom, angles[1]);
        Build(skeleton, 1, thumb, angles[1], skeleton.bones[1]);
    }
    return skeleton;
}

void Glide(HandSkeleton& drawn, const HandSkeleton& fitted, float seconds) {
    if (drawn.right != fitted.right) {
        drawn = fitted;
        return;
    }
    drawn.tipsApart = fitted.tipsApart;
    const float off = Length(fitted.wrist - drawn.wrist);
    const float share = 1 - std::exp(-seconds / GlideTime(off, kShiftStill, kShiftMoving));
    drawn.wrist = drawn.wrist + (fitted.wrist - drawn.wrist) * share;
    GlideAxes(drawn.palm, fitted.palm, seconds);
    for (int f = 0; f < HandSkeleton::kFingers; ++f) {
        for (int b = 0; b < HandSkeleton::kBones; ++b) {
            Bone& bone = drawn.bones[f][b];
            GlideAxes(bone.axes, fitted.bones[f][b].axes, seconds);
            bone.length = fitted.bones[f][b].length;
            bone.start = b == 0 ? Knuckle(drawn, f) : drawn.bones[f][b - 1].End();
        }
    }
}

}  // namespace hand_skeleton

}  // namespace gazeora
