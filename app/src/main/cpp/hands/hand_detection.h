#pragma once

#include <vector>

#include "hands/camera_frame.h"
#include "hands/hand_geometry.h"

namespace gazeora {

// A palm the palm detector found: how sure it is, its box in the upright picture (in pixels),
// and its seven keypoints there: the wrist's middle, the index, middle, ring and little
// fingers' knuckles, and the thumb's base and middle joint.
struct Palm {
    static constexpr int kKeypoints = 7;
    float score = 0;
    float x = 0;
    float y = 0;
    float width = 0;
    float height = 0;
    float keypoints[kKeypoints][2] = {};
};

// The two models of MediaPipe's hand landmarker (hand_detector.tflite and
// hand_landmarks_detector.tflite), read the way MediaPipe reads them: the palm detector looks
// at the whole picture to find hands; the landmark model, at a square around one hand, turned
// so its fingers point up, gives its landmarks and its 3D shape, and the square to look in
// next time.
namespace hand_detection {

// The models' inputs: square RGB pictures of these sides (camera_frame::CropRgb).
constexpr int kPalmSide = 192;
constexpr int kHandSide = 224;
// The palm detector's outputs: boxes of these many values, one per anchor; and a score each.
constexpr int kAnchors = 2016;
constexpr int kBoxValues = 18;
// The landmark model's: each landmark's x, y and z.
constexpr int kLandmarkValues = HandLandmarks::kCount * 3;
// The least scores for a palm, and for a hand to be there.
constexpr float kLeastPalmScore = 0.5f;
constexpr float kLeastPresence = 0.5f;

// The square the palm detector looks at for the whole `width` x `height` picture: as wide as
// its longer side, with black bars along the other.
PictureSquare WholePicture(int width, int height);

// The palms the detector found in `looked`, from its outputs (kAnchors x kBoxValues box
// values, and kAnchors scores before their sigmoid), best first; boxes overlapping a better
// one are blended into it.
std::vector<Palm> ReadPalms(const float* boxes, const float* scores, const PictureSquare& looked);

// Where to look for the landmarks of the hand that `palm` belongs to.
PictureSquare AroundPalm(const Palm& palm);

// Where to look for this hand next, from its landmarks in a `width` x `height` picture.
PictureSquare AroundHand(const HandLandmarks& hand, int width, int height);

// The hand the landmark model saw in `looked` of a `width` x `height` picture: from its
// landmarks (in the model's input's pixels), its 3D shape (in meters, as the square was
// turned) and its handedness score.
HandLandmarks ReadHand(const PictureSquare& looked, const float* landmarks, const float* shape,
                       float handedness, int width, int height);

// How much two squares overlap: their bounding boxes' intersection over their union.
float Overlap(const PictureSquare& a, const PictureSquare& b);

}  // namespace hand_detection

}  // namespace gazeora
