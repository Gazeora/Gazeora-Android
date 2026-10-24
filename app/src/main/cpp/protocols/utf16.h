#pragma once

#include <string>

namespace gazeora {

// `text`, read as UTF-8, in UTF-16: for a Java string (jni.cpp), by way of NewString rather than
// NewStringUTF, which takes only valid modified UTF-8 and aborts on anything else. The text
// comes off the network (a PC's name, an error in its answer) from whoever sent it, so what
// isn't UTF-8 (a stray byte, an overlong form, a surrogate) becomes U+FFFD instead.
std::u16string Utf16FromUtf8(const std::string& text);

}  // namespace gazeora
