#include "hands/hand_tracking.h"

#include <algorithm>
#include <cmath>

namespace gazeora {

namespace {

// Whether two squares are round the same hand: they overlap, or their middles are close.
bool SameHand(const PictureSquare& a, const PictureSquare& b) {
    return hand_detection::Overlap(a, b) > HandTracking::kSameHand ||
           std::hypot(a.x - b.x, a.y - b.y) <
               HandTracking::kSameMiddle * std::max(a.size, b.size);
}

}  // namespace

bool HandTracking::Searching(double now) const {
    if (m_squares.size() >= size_t(kMaxHands)) return false;
    return m_squares.empty() || now - m_searchedAt >= kSearchEvery;
}

void HandTracking::Search(const std::vector<Palm>& palms, double now) {
    m_searchedAt = now;
    for (const Palm& palm : palms) {
        if (m_squares.size() >= size_t(kMaxHands)) break;
        const PictureSquare square = hand_detection::AroundPalm(palm);
        bool followed = false;
        for (const PictureSquare& other : m_squares) {
            followed = followed || SameHand(square, other);
        }
        if (!followed) m_squares.push_back(square);
    }
    m_presence.assign(m_squares.size(), 0);
    m_hands.assign(m_squares.size(), HandLandmarks());
    m_next.assign(m_squares.size(), PictureSquare());
}

void HandTracking::Answer(size_t square, float presence, const HandLandmarks& hand, int width,
                          int height) {
    if (m_presence.size() != m_squares.size()) {
        m_presence.assign(m_squares.size(), 0);
        m_hands.assign(m_squares.size(), HandLandmarks());
        m_next.assign(m_squares.size(), PictureSquare());
    }
    if (square >= m_squares.size()) return;
    m_presence[square] = presence;
    m_hands[square] = hand;
    m_next[square] = hand_detection::AroundHand(hand, width, height);
}

std::vector<HandLandmarks> HandTracking::Finish() {
    std::vector<HandLandmarks> hands;
    std::vector<PictureSquare> next;
    std::vector<float> presence;
    m_presence.resize(m_squares.size(), 0);
    m_hands.resize(m_squares.size());
    m_next.resize(m_squares.size());
    for (size_t i = 0; i < m_squares.size(); ++i) {
        if (m_presence[i] < hand_detection::kLeastPresence) continue;
        // Two squares that slid onto the same hand: the surer one keeps it.
        bool same = false;
        for (size_t k = 0; k < next.size() && !same; ++k) {
            if (SameHand(m_next[i], next[k])) {
                same = true;
                if (m_presence[i] > presence[k]) {
                    next[k] = m_next[i];
                    hands[k] = m_hands[i];
                    presence[k] = m_presence[i];
                }
            }
        }
        if (same) continue;
        next.push_back(m_next[i]);
        hands.push_back(m_hands[i]);
        presence.push_back(m_presence[i]);
    }
    m_squares = std::move(next);
    m_presence.assign(m_squares.size(), 0);
    m_hands.assign(m_squares.size(), HandLandmarks());
    m_next.assign(m_squares.size(), PictureSquare());
    return hands;
}

void HandTracking::Clear() {
    m_squares.clear();
    m_presence.clear();
    m_hands.clear();
    m_next.clear();
    m_searchedAt = -1e9;
}

}  // namespace gazeora
