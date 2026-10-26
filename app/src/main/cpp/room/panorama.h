#pragma once

#include <GLES2/gl2.h>

#include <cstdint>
#include <utility>
#include <vector>

namespace gazeora {

// The 360 scene behind the window when there's no camera: RGBA pixels from the app
// (assets/room/panorama.jpg), made into a texture the first time it's drawn, then let go of.
// Used on the render thread.
class Panorama {
public:
    // Tightly packed RGBA, rows top first; the width a power of two, to wrap around.
    struct Pixels {
        std::vector<uint8_t> rgba;
        int32_t width = 0;
        int32_t height = 0;
    };

    Panorama() = default;
    ~Panorama() { Release(); }
    Panorama(const Panorama&) = delete;
    Panorama& operator=(const Panorama&) = delete;

    // Whether to ask the app for the pixels: once, the first time the scene is needed.
    bool TakeRequest();
    void SetPixels(Pixels pixels) { m_pixels = std::move(pixels); }
    // Binds the texture to the active unit; false until there are pixels.
    bool Bind();
    // Before the context goes.
    void Release();

private:
    bool Upload();

    Pixels m_pixels;
    GLuint m_texture = 0;
    bool m_requested = false;
};

}  // namespace gazeora
