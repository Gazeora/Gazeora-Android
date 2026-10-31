#include "hands/rigged_hand.h"

#include <iterator>

#include "hands/hand_model_data.h"

namespace gazeora {

namespace {

// The model's joints, by landmark: the wrist, then each finger's from its base out.
Vec3 Joint(int landmark) {
    const float* at = hand_model::kJoints[landmark];
    return {at[0], at[1], at[2]};
}

// The skeleton of the model's rest pose: its palm on the axes the model's data is in, and
// each bone from its joint to the next, with the axes the skeleton's fit gives a bone.
HandSkeleton RestSkeleton() {
    HandSkeleton rest;
    rest.right = true;
    // palmward = across x along, as a right hand's is; the data has z back from the palm
    rest.palm = {{1, 0, 0}, {0, 0, -1}, {0, 1, 0}};
    for (int f = 0; f < HandSkeleton::kFingers; ++f) {
        for (int b = 0; b < HandSkeleton::kBones; ++b) {
            const Vec3 start = Joint(1 + f * 4 + b), end = Joint(2 + f * 4 + b);
            const Vec3 along = Normalize(end - start, rest.palm.along);
            Bone& bone = rest.bones[f][b];
            bone.start = start;
            bone.length = Length(end - start);
            if (b > 0) {
                bone.axes = hand_skeleton::NextAxes(rest.bones[f][b - 1].axes, along);
            } else if (f == 0) {
                bone.axes = hand_skeleton::ThumbAxes(rest.palm, along);
            } else {
                bone.axes = hand_skeleton::FingerAxes(rest.palm, along);
            }
        }
    }
    return rest;
}

// The bone `bone` (RiggedHand's numbering) of `skeleton`: where it starts, and its axes.
void Frame(const HandSkeleton& skeleton, int bone, Vec3& origin, Axes& axes) {
    if (bone == 0) {
        origin = skeleton.wrist;
        axes = skeleton.palm;
        return;
    }
    const int f = (bone - 1) / HandSkeleton::kBones, b = (bone - 1) % HandSkeleton::kBones;
    origin = skeleton.bones[f][b].start;
    axes = skeleton.bones[f][b].axes;
    // The thumb's across comes from its curl, not from the palm's: mirrored with the hand.
    if (f == 0 && !skeleton.right) {
        axes.across = -axes.across;
    }
}

Vec3 InAxes(Vec3 v, const Axes& axes) {
    return {Dot(v, axes.along), Dot(v, axes.palmward), Dot(v, axes.across)};
}

Vec3 OutOfAxes(Vec3 v, const Axes& axes) {
    return axes.along * v.x + axes.palmward * v.y + axes.across * v.z;
}

}  // namespace

const RiggedHand& RiggedHand::Get() {
    static const RiggedHand hand;
    return hand;
}

RiggedHand::RiggedHand() : m_rest(RestSkeleton()) {
    Vec3 origin[kBones];
    Axes axes[kBones];
    for (int bone = 0; bone < kBones; ++bone) {
        Frame(m_rest, bone, origin[bone], axes[bone]);
    }
    m_corners.reserve(std::size(hand_model::kCorners));
    for (const hand_model::Corner& model : hand_model::kCorners) {
        Corner corner;
        const Vec3 at = {model.x, model.y, model.z}, normal = {model.nx, model.ny, model.nz};
        for (int i = 0; i < kInfluences; ++i) {
            if (model.weights[i] <= 0) continue;
            Influence& influence = corner.influences[corner.count++];
            influence.bone = model.bones[i];
            influence.weight = model.weights[i];
            influence.at = InAxes(at - origin[influence.bone], axes[influence.bone]);
            influence.normal = InAxes(normal, axes[influence.bone]);
        }
        corner.shows = model.shows;
        m_corners.push_back(corner);
    }
    m_triangles.assign(std::begin(hand_model::kTriangles), std::end(hand_model::kTriangles));
}

void RiggedHand::Pose(const HandSkeleton& skeleton, std::vector<HandVertex>& corners,
                      const float* glow) const {
    Vec3 origin[kBones];
    Axes axes[kBones];
    for (int bone = 0; bone < kBones; ++bone) {
        Frame(skeleton, bone, origin[bone], axes[bone]);
    }
    corners.resize(m_corners.size());
    for (size_t v = 0; v < m_corners.size(); ++v) {
        const Corner& corner = m_corners[v];
        Vec3 at, normal;
        float glows = 0;
        for (int i = 0; i < corner.count; ++i) {
            const Influence& influence = corner.influences[i];
            const Axes& a = axes[influence.bone];
            at = at + (origin[influence.bone] + OutOfAxes(influence.at, a)) * influence.weight;
            normal = normal + OutOfAxes(influence.normal, a) * influence.weight;
            if (glow) glows += glow[influence.bone] * influence.weight;
        }
        normal = Normalize(normal, axes[0].palmward);
        HandVertex& out = corners[v];
        out = {at.x, at.y, at.z, normal.x, normal.y, normal.z, corner.shows};
        out.glow = glows;
    }
}

}  // namespace gazeora
