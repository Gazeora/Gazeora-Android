#include "room/panorama.h"

#include <android/log.h>

namespace gazeora {

namespace {

constexpr char kTag[] = "GazeoraPanorama";
constexpr size_t kBytesPerPixel = 4;

bool PowerOfTwo(int32_t value) { return value > 0 && (value & (value - 1)) == 0; }

}  // namespace

bool Panorama::TakeRequest() {
    if (m_requested || m_texture) {
        return false;
    }
    m_requested = true;
    return true;
}

bool Panorama::Bind() {
    if (!m_texture && !Upload()) {
        return false;
    }
    glBindTexture(GL_TEXTURE_2D, m_texture);
    return true;
}

void Panorama::Release() {
    if (m_texture) {
        glDeleteTextures(1, &m_texture);
        m_texture = 0;
    }
}

bool Panorama::Upload() {
    const Pixels& p = m_pixels;
    if (p.width <= 0 || p.height <= 0 ||
        p.rgba.size() < size_t(p.width) * size_t(p.height) * kBytesPerPixel) {
        return false;
    }
    GLint maxSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
    if (p.width > maxSize || p.height > maxSize) {
        __android_log_print(ANDROID_LOG_WARN, kTag, "%dx%d is beyond this GPU's %d", p.width,
                            p.height, maxSize);
        m_pixels = Pixels();
        return false;
    }
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Around the horizon it wraps; ES 2 repeats power-of-two textures only.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                    PowerOfTwo(p.width) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, p.width, p.height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 p.rgba.data());
    m_pixels = Pixels();  // the GPU has its own copy
    return true;
}

}  // namespace gazeora
