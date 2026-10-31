#include "hands/hand_follower.h"

#include <algorithm>
#include <cmath>

namespace gazeora {

namespace {

constexpr float kTwoPi = 6.2831853f;
// How far a hand moves between two answers and is still the same one, in its palm lengths.
constexpr float kSameHand = 1.5f;
// A hand seen this near a followed one, in palm lengths, is the tracker seeing that one twice:
// the viewer's two hands' palms can't be on top of each other.
constexpr float kOneHand = 0.6f;
// A followed hand drawn back farther than this, in arm's reaches, isn't the viewer's after all.
constexpr float kFollowedReach = 1.3f;
// The smoothing's cutoff, in Hz: this while the hand is still, and this much more per palm
// length a second it moves (a One Euro filter).
constexpr float kStillCutoff = 1.5f;
constexpr float kCutoffPerSpeed = 0.6f;
// Whether a hand just found is the viewer's right or left: how much the way its arm goes from
// the wrist counts (across, as a share of the way), where it is across the view (from -1 at the
// left edge to 1 at the right), and the tracker's guess.
constexpr float kArmCounts = 1.0f;
constexpr float kPlaceCounts = 0.5f;
constexpr float kGuessCounts = 0.3f;
// A shape at least this right- or left-handed (hand_geometry::Rightness) says which hand the
// tracker took it for; a flatter one, its guess does.
constexpr float kClearlyHanded = 0.1f;

// Whether `hand`, just found, is the viewer's right hand. Their right arm comes in from the
// right, and the right hand is mostly on the right. The tracker's guess counts for less: from
// behind, a right hand looks much like a left one from the front, and it mixes them up.
bool IsRight(const ShownHand& hand, float tanAcross) {
    const ViewTangent& wrist = hand.points[HandLandmarks::kWrist];
    const ViewTangent& knuckle = hand.points[HandLandmarks::kMiddleBase];
    const float length = std::hypot(wrist.x - knuckle.x, wrist.y - knuckle.y);
    const float arm = length > 0 ? (wrist.x - knuckle.x) / length : 0;
    const float place = tanAcross > 0 ? std::clamp(wrist.x / tanAcross, -1.0f, 1.0f) : 0;
    return kArmCounts * arm + kPlaceCounts * place + (hand.right ? kGuessCounts : -kGuessCounts) >
           0;
}

// `seen` as the viewer's `right` hand. Where the tracker took it for the other (by its 3D
// shape, or its guess where that's flat), it took it for that hand turned about: the same in
// the picture, with its depths the other way about the wrist.
ShownHand AsSide(ShownHand seen, bool right) {
    const float rightness = seen.solid ? hand_geometry::Rightness(seen) : 0;
    const bool takenRight =
        std::fabs(rightness) >= kClearlyHanded ? rightness > 0 : seen.right;
    if (takenRight != right) {
        for (float& depth : seen.depth) depth = -depth;
    }
    seen.right = right;
    return seen;
}

ViewTangent Middle(const ShownHand& hand) {
    const ViewTangent& wrist = hand.points[HandLandmarks::kWrist];
    const ViewTangent& knuckle = hand.points[HandLandmarks::kMiddleBase];
    return {(wrist.x + knuckle.x) / 2, (wrist.y + knuckle.y) / 2};
}

float Apart(const ShownHand& a, const ShownHand& b) {
    ViewTangent ma = Middle(a), mb = Middle(b);
    return std::hypot(ma.x - mb.x, ma.y - mb.y);
}

// Moves `hand` `alpha` of the way to `toward`.
void Blend(ShownHand& hand, const ShownHand& toward, float alpha) {
    for (int i = 0; i < HandLandmarks::kCount; ++i) {
        hand.points[i].x += (toward.points[i].x - hand.points[i].x) * alpha;
        hand.points[i].y += (toward.points[i].y - hand.points[i].y) * alpha;
        hand.depth[i] += (toward.depth[i] - hand.depth[i]) * alpha;
    }
    hand.palm += (toward.palm - hand.palm) * alpha;
    hand.solid = toward.solid;
}

// Moves `hand` toward the newer `seen`, `dt` seconds on.
void Smooth(ShownHand& hand, const ShownHand& seen, float dt) {
    float speed = dt > 0 ? Apart(hand, seen) / dt / std::max(seen.palm, 1e-6f) : 0;
    float cutoff = kStillCutoff + kCutoffPerSpeed * speed;
    Blend(hand, seen, dt > 0 ? 1 - std::exp(-kTwoPi * cutoff * dt) : 1);
}

}  // namespace

void HandFollower::Update(const std::vector<ShownHand>& seen, float tanAcross, float tanDown,
                          double now) {
    m_followed.erase(std::remove_if(m_followed.begin(), m_followed.end(),
                                    [now](const Followed& f) { return now - f.seenAt > kLostAfter; }),
                     m_followed.end());
    // Each followed hand takes the nearest hand seen close enough, closest pairs first.
    struct Pair {
        float apart;
        size_t followed;
        size_t seen;
    };
    std::vector<Pair> pairs;
    for (size_t f = 0; f < m_followed.size(); ++f) {
        for (size_t s = 0; s < seen.size(); ++s) {
            float apart = Apart(m_followed[f].hand, seen[s]);
            if (apart <= kSameHand * std::max(m_followed[f].hand.palm, seen[s].palm)) {
                pairs.push_back({apart, f, s});
            }
        }
    }
    std::sort(pairs.begin(), pairs.end(),
              [](const Pair& a, const Pair& b) { return a.apart < b.apart; });
    std::vector<bool> followedTaken(m_followed.size()), seenTaken(seen.size());
    for (const Pair& pair : pairs) {
        if (followedTaken[pair.followed] || seenTaken[pair.seen]) continue;
        followedTaken[pair.followed] = seenTaken[pair.seen] = true;
        Followed& followed = m_followed[pair.followed];
        Smooth(followed.hand, AsSide(seen[pair.seen], followed.hand.right),
               float(now - followed.seenAt));
        followed.fitted = hand_skeleton::Fit(followed.hand);
        followed.seenAt = now;
    }
    m_followed.erase(std::remove_if(m_followed.begin(), m_followed.end(),
                                    [](const Followed& f) {
                                        return f.hand.palm * kFollowedReach <
                                               hand_geometry::kPalmLength /
                                                   hand_geometry::kArmsReach;
                                    }),
                     m_followed.end());
    // New hands, only the viewer's, while there's room; not where one is followed already,
    // which would be the tracker seeing one hand twice.
    for (size_t s = 0; s < seen.size(); ++s) {
        if (seenTaken[s] || m_followed.size() >= size_t(kMaxHands)) continue;
        bool followedThere = false;
        for (const Followed& followed : m_followed) {
            followedThere = followedThere ||
                            Apart(followed.hand, seen[s]) <=
                                kOneHand * std::max(followed.hand.palm, seen[s].palm);
        }
        if (followedThere) continue;
        if (hand_geometry::WithinReach(seen[s]) &&
            hand_geometry::ComesFromViewer(seen[s], tanAcross, tanDown)) {
            // With one hand followed already, this is the other.
            const bool right = m_followed.empty() ? IsRight(seen[s], tanAcross)
                                                  : !m_followed.front().hand.right;
            Followed followed;
            followed.hand = AsSide(seen[s], right);
            followed.seenAt = now;
            followed.drawnAt = now;
            followed.fitted = hand_skeleton::Fit(followed.hand);
            followed.drawn = followed.fitted;
            m_followed.push_back(followed);
        }
    }
}

std::vector<ShownHand> HandFollower::Hands(double now) const {
    std::vector<ShownHand> hands;
    for (const Followed& followed : m_followed) {
        if (now - followed.seenAt <= kLostAfter) {
            hands.push_back(followed.hand);
        }
    }
    std::sort(hands.begin(), hands.end(),
              [](const ShownHand& a, const ShownHand& b) { return a.palm > b.palm; });
    return hands;
}

std::vector<HandSkeleton> HandFollower::Skeletons(double now) {
    std::vector<const Followed*> shown;
    for (Followed& followed : m_followed) {
        if (now - followed.seenAt > kLostAfter) continue;
        hand_skeleton::Glide(followed.drawn, followed.fitted,
                             float(std::max(now - followed.drawnAt, 0.0)));
        followed.drawnAt = now;
        shown.push_back(&followed);
    }
    std::sort(shown.begin(), shown.end(), [](const Followed* a, const Followed* b) {
        return a->hand.palm > b->hand.palm;
    });
    std::vector<HandSkeleton> skeletons;
    for (const Followed* followed : shown) {
        skeletons.push_back(followed->drawn);
    }
    return skeletons;
}

}  // namespace gazeora
