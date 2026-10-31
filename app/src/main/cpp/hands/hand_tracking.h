#pragma once

#include <vector>

#include "hands/camera_frame.h"
#include "hands/hand_detection.h"
#include "hands/hand_geometry.h"

namespace gazeora {

// Which parts of each camera picture the hand tracker looks at, as VR headsets track hands:
// the palm detector goes over the whole picture only to find hands, now and then while it
// follows fewer than it can; each hand found is then followed by the landmark model alone,
// in a square around where it was last. For each picture: Search if Searching says to, then
// Answer for each of Squares, then Finish. Times in seconds, from any one clock.
class HandTracking {
public:
    // At most the viewer's two hands.
    static constexpr int kMaxHands = 2;
    // How often it looks for another hand while it follows one.
    static constexpr double kSearchEvery = 0.25;
    // A palm found overlapping a followed hand's square this much, or with its middle this
    // near the square's (as a share of the square), is that hand.
    static constexpr float kSameHand = 0.5f;
    static constexpr float kSameMiddle = 0.25f;

    // Whether to look for new hands in the picture at `now`.
    bool Searching(double now) const;
    // The palms the detector found at `now`: those not already followed are followed.
    void Search(const std::vector<Palm>& palms, double now);
    // Where to look for each hand followed.
    const std::vector<PictureSquare>& Squares() const { return m_squares; }
    // The landmark model's answer for square `square` of a `width` x `height` picture: how sure
    // it is the hand is there, and the hand it saw (hand_detection::ReadHand).
    void Answer(size_t square, float presence, const HandLandmarks& hand, int width, int height);
    // The hands in this picture; those it lost are no longer followed.
    std::vector<HandLandmarks> Finish();
    void Clear();

private:
    std::vector<PictureSquare> m_squares;
    // For each square, this picture's.
    std::vector<float> m_presence;
    std::vector<HandLandmarks> m_hands;
    std::vector<PictureSquare> m_next;
    double m_searchedAt = -1e9;
};

}  // namespace gazeora
