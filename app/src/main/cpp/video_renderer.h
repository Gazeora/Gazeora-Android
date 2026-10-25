#pragma once

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <android/hardware_buffer.h>
#include <android/native_window.h>
#include <media/NdkImageReader.h>

#include <condition_variable>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "protocols/view_info.h"
#include "shaders/shader_program.h"

namespace gazeora {

// Shows the decoder's pictures on a window with OpenGL ES, the way each frame's view info says
// (protocols/view_info.h): as it is, fitted to the screen with black bars, or for a VR viewer,
// in both eyes and barrel-distorted for the lenses, each eye its own view of a pair. The
// decoder draws into an image reader; each picture is drawn the moment it arrives, on the
// renderer's own thread.
class VideoRenderer {
public:
    // `window` must outlive the renderer.
    explicit VideoRenderer(ANativeWindow* window);
    ~VideoRenderer();
    VideoRenderer(const VideoRenderer&) = delete;
    VideoRenderer& operator=(const VideoRenderer&) = delete;

    // Sets up OpenGL ES on the window, and the image reader. False with `error` if this device
    // can't; the decoder can draw straight to the window instead.
    bool Start(std::string& error);
    // Where the decoder draws, once started. The reader's: it lives as long as the renderer.
    ANativeWindow* DecoderWindow() const { return m_readerWindow; }
    // How to show the picture with this presentation time, from the decoder's thread.
    void Expect(int64_t presentationUs, const ViewInfo& view);

private:
    // A decoder buffer as a texture, made once per buffer: the decoder cycles through a few.
    struct Texture {
        AHardwareBuffer* buffer = nullptr;  // a reference of ours
        EGLImageKHR image = EGL_NO_IMAGE_KHR;
        GLuint texture = 0;
        uint32_t width = 0;
        uint32_t height = 0;
    };
    // The picture last drawn, to draw again when the window's size changes.
    struct Shown {
        AHardwareBuffer* buffer = nullptr;  // its texture's
        AImageCropRect crop = {};
        ViewInfo view;
    };

    static void OnImageAvailable(void* context, AImageReader* reader);
    void Run();
    bool SetUpEgl(std::string& error);
    bool SetUpGl(std::string& error);
    void TearDown();
    void DrawLatest();
    void Draw(const Texture& texture, const Shown& shown);
    // The buffer's texture, made if it's new; null if it can't be.
    const Texture* TextureFor(AHardwareBuffer* buffer, bool make);
    ViewInfo ViewFor(int64_t presentationUs);

    ANativeWindow* m_window;
    AImageReader* m_reader = nullptr;
    ANativeWindow* m_readerWindow = nullptr;  // the reader's

    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_wake;
    bool m_stopping = false;
    bool m_imageReady = false;
    bool m_startDone = false;
    std::string m_startError;  // with m_startDone: empty if it started
    std::map<int64_t, ViewInfo> m_views;  // by presentation time, from the decoder

    // The render thread's.
    EGLDisplay m_display = EGL_NO_DISPLAY;
    EGLContext m_context = EGL_NO_CONTEXT;
    EGLSurface m_surface = EGL_NO_SURFACE;
    PFNEGLGETNATIVECLIENTBUFFERANDROIDPROC m_getNativeClientBuffer = nullptr;
    PFNEGLCREATEIMAGEKHRPROC m_createImage = nullptr;
    PFNEGLDESTROYIMAGEKHRPROC m_destroyImage = nullptr;
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC m_imageTargetTexture = nullptr;
    PFNEGLCREATESYNCKHRPROC m_createSync = nullptr;
    PFNEGLDESTROYSYNCKHRPROC m_destroySync = nullptr;
    PFNEGLDUPNATIVEFENCEFDANDROIDPROC m_dupFence = nullptr;
    ShaderProgram m_program;
    GLint m_lens = -1;
    GLint m_eyeLayout = -1;
    GLint m_crop = -1;
    GLint m_pairView = -1;
    GLint m_bufferSize = -1;
    std::vector<Texture> m_textures;  // oldest first
    Shown m_shown;
    ViewInfo m_view;  // of the latest picture
    EGLint m_shownWidth = 0;
    EGLint m_shownHeight = 0;
};

}  // namespace gazeora
