// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

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
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "gl/egl_procs.h"
#include "gl/hardware_buffer_textures.h"
#include "gl/image_releaser.h"
#include "protocols/view_info.h"
#include "hands/hand_renderer.h"
#include "shaders/shader_program.h"

namespace gazeora {

// Shows the decoder's pictures on a window with OpenGL ES, the way each frame's view info says
// (protocols/view_info.h): as it is, fitted to the screen with black bars, or for a VR viewer,
// in both eyes and barrel-distorted for the lenses, each eye its own view of a pair. Gazeora's
// own window in a VR viewer has the viewer's hands over it (hands/hand_renderer.h), so it's
// redrawn every refresh as they move. The decoder draws into an image reader; each picture is
// drawn the moment it arrives, on the renderer's own thread.
class VideoRenderer {
public:
    // `window` must outlive the renderer. While Gazeora's window shows in a VR viewer (or flat
    // too, when debugging), the viewer's hands show over it as `hands` says.
    VideoRenderer(ANativeWindow* window, HandsOptions hands);
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
    // The display's turn (Surface.ROTATION_*) and density, from any thread.
    void SetScreen(int quarterTurns, float dotsPerInch);

private:
    // The picture last drawn, to draw again when the window's size changes or the hands move.
    struct Shown {
        AHardwareBuffer* buffer = nullptr;  // its texture's
        AImageCropRect crop = {};
        ViewInfo view;
    };

    static void OnImageAvailable(void* context, AImageReader* reader);
    void Run();
    // Waits for a picture, or a frame's time with the hands; false when stopping.
    bool Wait(bool& image);
    bool SetUpEgl(std::string& error);
    bool SetUpGl(std::string& error);
    void TearDown();
    void DrawLatest();
    void Draw(const HardwareBufferTextures::Texture& texture, const Shown& shown);
    void RedrawIfResized();
    bool ShowsHands(const ViewInfo& view) const;
    bool WithHands() const;
    void DrawWithHands();
    void Hold(AImage* image);
    void StopHands();
    ViewInfo ViewFor(int64_t presentationUs);

    ANativeWindow* m_window;
    HandsOptions m_handsOptions;  // until the hand renderer takes them
    const bool m_handsDebug;
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
    int m_quarterTurns = 1;
    float m_dotsPerInch = 0;

    // The render thread's.
    EGLDisplay m_display = EGL_NO_DISPLAY;
    EGLContext m_context = EGL_NO_CONTEXT;
    EGLSurface m_surface = EGL_NO_SURFACE;
    EglProcs m_procs;
    std::unique_ptr<HardwareBufferTextures> m_textures;
    std::unique_ptr<ImageReleaser> m_releaser;
    std::unique_ptr<HandRenderer> m_hands;  // null where the hands can't be drawn
    AImage* m_held = nullptr;  // with the hands, the picture shown, until a newer one comes
    bool m_swapFailed = false;
    ShaderProgram m_program;
    GLint m_lens = -1;
    GLint m_eyeLayout = -1;
    GLint m_crop = -1;
    GLint m_pairView = -1;
    GLint m_bufferSize = -1;
    GLint m_cursor = -1;
    GLint m_pictureAspect = -1;
    Shown m_shown;
    ViewInfo m_view;  // of the latest picture
    EGLint m_shownWidth = 0;
    EGLint m_shownHeight = 0;
};

}  // namespace gazeora
