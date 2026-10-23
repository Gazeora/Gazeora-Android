#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace gazeora {

// A touch on the picture as the PC's mouse should take it, sent while the picture is shown
// flat (not for a VR viewer). The numbers are on the wire. Click is a press and release in
// one message, so a lost datagram can't leave a button held.
enum class PointerAction : uint8_t { Move = 1, Press = 2, Release = 3, Scroll = 4, Click = 5 };

enum class PointerButton : uint8_t { Left = 0, Right = 1 };

struct PointerEvent {
    PointerAction action = PointerAction::Move;
    PointerButton button = PointerButton::Left;
    // Across the picture, 0..1 from its top left.
    float x = 0.0f;
    float y = 0.0f;
    // Scroll only, in Windows' wheel units (120 a notch): positive scrolls up and right.
    int wheelVertical = 0;
    int wheelHorizontal = 0;
};

// A GZF1 Pointer message (gzf.h): the header, its last word the payload length, then a
// 10-byte big-endian payload: action, button, x and y as u16 0..65535, vertical and
// horizontal wheel as i16. The PC has its own copy (Gazeora: pointer_message.h); keep the
// two in step.
constexpr size_t kPointerPayloadSize = 10;
// Where pointer messages go over the network, whatever the stream's protocol.
constexpr uint16_t kPointerPort = 8557;

std::vector<uint8_t> PointerMessage(const PointerEvent& event);

}  // namespace gazeora
