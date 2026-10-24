#include "protocols/utf16.h"

#include <cstddef>
#include <cstdint>

namespace gazeora {

namespace {

constexpr uint32_t kReplacement = 0xFFFD;

// The UTF-8 sequence at `p` (`left` bytes on) as a code point, and how long it is: U+FFFD and
// 1 for a byte that doesn't start one, or starts one cut short; U+FFFD and its length for one
// that isn't a character.
uint32_t Decode(const uint8_t* p, size_t left, size_t& length) {
    static const uint32_t kSmallest[] = {0, 0, 0x80, 0x800, 0x10000};
    uint32_t first = p[0];
    length = first < 0x80 ? 1 : (first >> 5) == 0x6 ? 2 : (first >> 4) == 0xE ? 3
                              : (first >> 3) == 0x1E ? 4 : 0;
    if (length == 1) {
        return first;
    }
    if (length == 0 || length > left) {
        length = 1;
        return kReplacement;
    }
    uint32_t code = first & (0xFFu >> (length + 1));
    for (size_t i = 1; i < length; ++i) {
        if ((p[i] & 0xC0) != 0x80) {
            length = 1;
            return kReplacement;
        }
        code = code << 6 | (p[i] & 0x3F);
    }
    // Overlong, a surrogate, or past Unicode.
    if (code < kSmallest[length] || code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF)) {
        return kReplacement;
    }
    return code;
}

}  // namespace

std::u16string Utf16FromUtf8(const std::string& text) {
    std::u16string utf16;
    utf16.reserve(text.size());
    const auto* bytes = reinterpret_cast<const uint8_t*>(text.data());
    for (size_t at = 0, length = 0; at < text.size(); at += length) {
        uint32_t code = Decode(bytes + at, text.size() - at, length);
        if (code >= 0x10000) {
            code -= 0x10000;
            utf16 += char16_t(0xD800 | (code >> 10));
            utf16 += char16_t(0xDC00 | (code & 0x3FF));
        } else {
            utf16 += char16_t(code);
        }
    }
    return utf16;
}

}  // namespace gazeora
