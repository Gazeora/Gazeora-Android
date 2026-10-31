#include "hands/hand_detection.h"

#include <algorithm>
#include <cmath>

namespace gazeora {

namespace hand_detection {

namespace {

constexpr float kPi = 3.14159265f;
// The detector's anchors: a grid of cells this many pixels apart, with this many anchors in
// each (MediaPipe's palm detection options: strides 8, 16, 16, 16, two anchors a layer).
constexpr int kStrides[] = {8, 16};
constexpr int kAnchorsPerCell[] = {2, 6};
// Scores are clipped to this before their sigmoid.
constexpr float kScoreClip = 100;
// Boxes overlapping a better one more than this are the same palm.
constexpr float kSamePalm = 0.3f;
// The square to look for the landmarks in: turned so the wrist is below the middle knuckle,
// as wide as the palm's box is long times this, moved this far toward the fingers.
constexpr float kPalmScale = 2.6f;
constexpr float kPalmShift = -0.5f;
// From the last landmarks: as wide as the palm and fingers' bases times this, moved this far.
constexpr float kHandScale = 2.0f;
constexpr float kHandShift = -0.1f;
// The landmarks the next square goes round: the wrist, and each finger's first two joints.
constexpr int kAroundLandmarks[] = {0, 1, 2, 3, 5, 6, 9, 10, 13, 14, 17, 18};
// The landmarks whose line from the wrist turns the square: between the index, middle and
// ring knuckles.
constexpr int kIndexKnuckle = 5;
constexpr int kMiddleKnuckle = 9;
constexpr int kRingKnuckle = 13;

struct Anchor {
    float x;
    float y;
};

const std::vector<Anchor>& Anchors() {
    static const std::vector<Anchor> anchors = [] {
        std::vector<Anchor> made;
        for (size_t layer = 0; layer < std::size(kStrides); ++layer) {
            const int cells = kPalmSide / kStrides[layer];
            for (int y = 0; y < cells; ++y) {
                for (int x = 0; x < cells; ++x) {
                    for (int k = 0; k < kAnchorsPerCell[layer]; ++k) {
                        made.push_back({(x + 0.5f) / cells, (y + 0.5f) / cells});
                    }
                }
            }
        }
        return made;
    }();
    return anchors;
}

float WrapAngle(float angle) {
    return angle - 2 * kPi * std::floor((angle + kPi) / (2 * kPi));
}

// The square's turn for a hand whose wrist is at `wrist` and the middle of its palm toward
// the fingers at `toward`: so the fingers point up in it.
float Turn(float wristX, float wristY, float towardX, float towardY) {
    return WrapAngle(kPi / 2 - std::atan2(-(towardY - wristY), towardX - wristX));
}

// A box around (x, y), `width` x `height` along its own axes turned `angle`, as a square to
// look in: moved `shift` of its height along its own y, its longer side times `scale`.
PictureSquare Square(float x, float y, float width, float height, float angle, float scale,
                     float shift) {
    PictureSquare square;
    square.x = x - height * shift * std::sin(angle);
    square.y = y + height * shift * std::cos(angle);
    square.size = std::max(width, height) * scale;
    square.angle = angle;
    return square;
}

// In the picture: where `looked`'s point (u, v) is, from 0 to 1 across and down it.
void FromSquare(const PictureSquare& looked, float u, float v, float& x, float& y) {
    const float c = std::cos(looked.angle), s = std::sin(looked.angle);
    const float across = (u - 0.5f) * looked.size, down = (v - 0.5f) * looked.size;
    x = looked.x + c * across - s * down;
    y = looked.y + s * across + c * down;
}

struct Box {
    float left, top, right, bottom;
};

Box BoundingBox(const PictureSquare& square) {
    const float half =
        square.size / 2 * (std::fabs(std::cos(square.angle)) + std::fabs(std::sin(square.angle)));
    return {square.x - half, square.y - half, square.x + half, square.y + half};
}

float Overlap(const Box& a, const Box& b) {
    const float across = std::min(a.right, b.right) - std::max(a.left, b.left);
    const float down = std::min(a.bottom, b.bottom) - std::max(a.top, b.top);
    if (across <= 0 || down <= 0) return 0;
    const float both = across * down;
    const float either = (a.right - a.left) * (a.bottom - a.top) +
                         (b.right - b.left) * (b.bottom - b.top) - both;
    return either > 0 ? both / either : 0;
}

Box PalmBox(const Palm& palm) {
    return {palm.x - palm.width / 2, palm.y - palm.height / 2, palm.x + palm.width / 2,
            palm.y + palm.height / 2};
}

}  // namespace

PictureSquare WholePicture(int width, int height) {
    PictureSquare square;
    square.x = width / 2.0f;
    square.y = height / 2.0f;
    square.size = float(std::max(width, height));
    return square;
}

std::vector<Palm> ReadPalms(const float* boxes, const float* scores, const PictureSquare& looked) {
    const std::vector<Anchor>& anchors = Anchors();
    std::vector<Palm> found;
    for (int i = 0; i < kAnchors; ++i) {
        const float score =
            1 / (1 + std::exp(-std::clamp(scores[i], -kScoreClip, kScoreClip)));
        if (score < kLeastPalmScore) continue;
        const float* box = boxes + i * kBoxValues;
        Palm palm;
        palm.score = score;
        FromSquare(looked, anchors[i].x + box[0] / kPalmSide, anchors[i].y + box[1] / kPalmSide,
                   palm.x, palm.y);
        palm.width = box[2] / kPalmSide * looked.size;
        palm.height = box[3] / kPalmSide * looked.size;
        for (int k = 0; k < Palm::kKeypoints; ++k) {
            FromSquare(looked, anchors[i].x + box[4 + 2 * k] / kPalmSide,
                       anchors[i].y + box[5 + 2 * k] / kPalmSide, palm.keypoints[k][0],
                       palm.keypoints[k][1]);
        }
        found.push_back(palm);
    }
    std::sort(found.begin(), found.end(),
              [](const Palm& a, const Palm& b) { return a.score > b.score; });
    // Each best palm left takes in those overlapping it, blended by their scores.
    std::vector<Palm> palms;
    std::vector<bool> taken(found.size());
    for (size_t best = 0; best < found.size(); ++best) {
        if (taken[best]) continue;
        const Box bestBox = PalmBox(found[best]);
        Palm blended;
        blended.score = found[best].score;
        float total = 0;
        for (size_t other = best; other < found.size(); ++other) {
            if (taken[other] || Overlap(bestBox, PalmBox(found[other])) <= kSamePalm) continue;
            taken[other] = true;
            const Palm& p = found[other];
            total += p.score;
            blended.x += p.x * p.score;
            blended.y += p.y * p.score;
            blended.width += p.width * p.score;
            blended.height += p.height * p.score;
            for (int k = 0; k < Palm::kKeypoints; ++k) {
                blended.keypoints[k][0] += p.keypoints[k][0] * p.score;
                blended.keypoints[k][1] += p.keypoints[k][1] * p.score;
            }
        }
        blended.x /= total;
        blended.y /= total;
        blended.width /= total;
        blended.height /= total;
        for (auto& keypoint : blended.keypoints) {
            keypoint[0] /= total;
            keypoint[1] /= total;
        }
        palms.push_back(blended);
    }
    return palms;
}

PictureSquare AroundPalm(const Palm& palm) {
    const float* wrist = palm.keypoints[0];
    const float* knuckle = palm.keypoints[2];
    return Square(palm.x, palm.y, palm.width, palm.height,
                  Turn(wrist[0], wrist[1], knuckle[0], knuckle[1]), kPalmScale, kPalmShift);
}

PictureSquare AroundHand(const HandLandmarks& hand, int width, int height) {
    auto x = [&](int i) { return hand.image[i][0] * width; };
    auto y = [&](int i) { return hand.image[i][1] * height; };
    const float towardX = ((x(kIndexKnuckle) + x(kRingKnuckle)) / 2 + x(kMiddleKnuckle)) / 2;
    const float towardY = ((y(kIndexKnuckle) + y(kRingKnuckle)) / 2 + y(kMiddleKnuckle)) / 2;
    const float angle = Turn(x(HandLandmarks::kWrist), y(HandLandmarks::kWrist), towardX, towardY);
    // The landmarks' bounding box along the square's own axes, about their middle.
    float left = x(0), right = left, top = y(0), bottom = top;
    for (int i : kAroundLandmarks) {
        left = std::min(left, x(i));
        right = std::max(right, x(i));
        top = std::min(top, y(i));
        bottom = std::max(bottom, y(i));
    }
    const float middleX = (left + right) / 2, middleY = (top + bottom) / 2;
    const float c = std::cos(angle), s = std::sin(angle);
    float least[2] = {1e9f, 1e9f}, most[2] = {-1e9f, -1e9f};
    for (int i : kAroundLandmarks) {
        const float dx = x(i) - middleX, dy = y(i) - middleY;
        const float along[2] = {c * dx + s * dy, -s * dx + c * dy};
        for (int k = 0; k < 2; ++k) {
            least[k] = std::min(least[k], along[k]);
            most[k] = std::max(most[k], along[k]);
        }
    }
    const float centerAcross = (least[0] + most[0]) / 2, centerDown = (least[1] + most[1]) / 2;
    return Square(middleX + c * centerAcross - s * centerDown,
                  middleY + s * centerAcross + c * centerDown, most[0] - least[0],
                  most[1] - least[1], angle, kHandScale, kHandShift);
}

HandLandmarks ReadHand(const PictureSquare& looked, const float* landmarks, const float* shape,
                       float handedness, int width, int height) {
    HandLandmarks hand;
    const float c = std::cos(looked.angle), s = std::sin(looked.angle);
    for (int i = 0; i < HandLandmarks::kCount; ++i) {
        const float* at = landmarks + 3 * i;
        float x, y;
        FromSquare(looked, at[0] / kHandSide, at[1] / kHandSide, x, y);
        hand.image[i][0] = x / width;
        hand.image[i][1] = y / height;
        // Its 3D shape comes as the square was turned: turned back to the picture's way.
        const float* p = shape + 3 * i;
        hand.shape[i][0] = c * p[0] - s * p[1];
        hand.shape[i][1] = s * p[0] + c * p[1];
        hand.shape[i][2] = p[2];
    }
    // Above a half, the viewer's right hand, on the back camera's picture.
    hand.right = handedness > 0.5f;
    return hand;
}

float Overlap(const PictureSquare& a, const PictureSquare& b) {
    return Overlap(BoundingBox(a), BoundingBox(b));
}

}  // namespace hand_detection

}  // namespace gazeora
