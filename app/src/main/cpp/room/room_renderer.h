#pragma once

#include <media/NdkImage.h>

#include <memory>
#include <string>

#include "gl/hardware_buffer_textures.h"
#include "gl/image_releaser.h"
#include "protocols/view_info.h"
#include "room/camera_feed.h"
#include "room/head_pose.h"
#include "room/head_tracker.h"
#include "room/panorama.h"
#include "room/room_geometry.h"
#include "shaders/shader_program.h"

namespace gazeora {

// Facade over the room view: Gazeora's window floating in front of the viewer, over the
// back camera's picture (or the 360 scene), held in place as the head turns, drawn into both
// eyes through the viewer's lenses. Made, used and destroyed on the render thread, with its
// context current.
class RoomRenderer {
public:
    // Null with `error` where the room can't be drawn; the window then shows like a game.
    static std::unique_ptr<RoomRenderer> Create(EGLDisplay display, const EglProcs& procs,
                                                const ImageReleaser& releaser, std::string& error);
    RoomRenderer(const RoomRenderer&) = delete;
    RoomRenderer& operator=(const RoomRenderer&) = delete;

    // The window appeared: camera and sensor on, and the room faces where the head does.
    void Enter();
    // Back to games: camera and sensor off.
    void Leave();
    bool Active() const { return m_active; }

    void SetScreen(int quarterTurns, float dotsPerInch);
    // True once, when the 360 scene's pixels are wanted from the app.
    bool TakePanoramaRequest();
    void SetPanorama(Panorama::Pixels pixels) { m_panorama.SetPixels(std::move(pixels)); }

    // Both eyes onto the surface, `width` x `height`, with `picture` in the window.
    void Draw(const HardwareBufferTextures::Texture& picture, const AImageCropRect& crop,
              const ViewInfo& view, EGLint width, EGLint height);

private:
    struct Uniforms {
        GLint lens = -1;
        GLint eyeLayout = -1;
        GLint pixelTan = -1;
        GLint crop = -1;
        GLint pictureTap = -1;
        GLint screenToRoom = -1;
        GLint eye = -1;
        GLint panel = -1;
        GLint cameraCrop = -1;
        GLint viewToSensor = -1;
        GLint cameraTan = -1;
        GLint backdrop = -1;
    };

    RoomRenderer(EGLDisplay display, const EglProcs& procs, const ImageReleaser& releaser);
    bool Build(std::string& error);
    Mat3 HeadNow();
    // The shader's backdrop value; the camera stretched to `reach`.
    float BindBackdrop(const room_geometry::EyeReach& reach);
    void SetWindow(const HardwareBufferTextures::Texture& picture, const AImageCropRect& crop,
                   float tanPerUnit, EGLint height);

    ShaderProgram m_program;
    Uniforms m_uniforms;
    HeadTracker m_head;
    RoomAnchor m_anchor;
    CameraFeed m_camera;
    Panorama m_panorama;
    bool m_active = false;
    int m_quarterTurns = 1;
    float m_dotsPerInch = 0;
};

}  // namespace gazeora
