#include "video_renderer.h"

#include <android/log.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iterator>

#include "shaders/video.frag.h"
#include "shaders/video.vert.h"

namespace gazeora {

namespace {

// The hand pointer's cursor's radius, as a share of the picture's height.
constexpr float kCursorRadius = 0.018f;

constexpr char kTag[] = "GazeoraRenderer";
// The decoder's pictures in flight: one it's drawing, one queued, one being shown, and one
// held while the hands show, as the shown one is drawn again every refresh.
constexpr int32_t kReaderImages = 5;
// Buffers whose textures are kept; the decoder cycles through fewer.
constexpr size_t kMaxTextures = 8;
// How often to look for a change in the window's size while no pictures come.
constexpr auto kIdleCheck = std::chrono::milliseconds(100);
// With the hands, frames go out as fast as the display takes them (eglSwapBuffers waits).
constexpr auto kNoWait = std::chrono::milliseconds(0);
// View infos waiting for their picture; more means the pictures went elsewhere.
constexpr size_t kMaxViews = 64;

const GLfloat kQuad[] = {-1, -1, 1, -1, -1, 1, 1, 1};

}  // namespace

VideoRenderer::VideoRenderer(ANativeWindow* window, HandsOptions hands)
    : m_window(window), m_handsDebug(hands.debug) {
    m_handsOptions = std::move(hands);
}

VideoRenderer::~VideoRenderer() {
    if (m_thread.joinable()) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stopping = true;
        }
        m_wake.notify_all();
        m_thread.join();
    }
    if (m_reader) {
        AImageReader_setImageListener(m_reader, nullptr);
        AImageReader_delete(m_reader);
    }
}

bool VideoRenderer::Start(std::string& error) {
    // The size is a default: the decoder sets its pictures' own.
    media_status_t status = AImageReader_newWithUsage(
        1920, 1080, AIMAGE_FORMAT_PRIVATE, AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE, kReaderImages,
        &m_reader);
    if (status != AMEDIA_OK || AImageReader_getWindow(m_reader, &m_readerWindow) != AMEDIA_OK) {
        error = "no image reader (error " + std::to_string(status) + ")";
        return false;
    }
    AImageReader_ImageListener listener = {this, &VideoRenderer::OnImageAvailable};
    AImageReader_setImageListener(m_reader, &listener);

    m_thread = std::thread([this] { Run(); });
    std::unique_lock<std::mutex> lock(m_mutex);
    m_wake.wait(lock, [this] { return m_startDone; });
    error = m_startError;
    return error.empty();
}

void VideoRenderer::Expect(int64_t presentationUs, const ViewInfo& view) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_views[presentationUs] = view;
    while (m_views.size() > kMaxViews) {
        m_views.erase(m_views.begin());
    }
}

void VideoRenderer::SetScreen(int quarterTurns, float dotsPerInch) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_quarterTurns = quarterTurns;
    m_dotsPerInch = dotsPerInch;
}

ViewInfo VideoRenderer::ViewFor(int64_t presentationUs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_views.find(presentationUs);
    if (it != m_views.end()) {
        m_view = it->second;
        m_views.erase(m_views.begin(), std::next(it));
    }
    return m_view;  // a picture it can't place keeps the one before's
}

void VideoRenderer::OnImageAvailable(void* context, AImageReader*) {
    auto* self = static_cast<VideoRenderer*>(context);
    {
        std::lock_guard<std::mutex> lock(self->m_mutex);
        self->m_imageReady = true;
    }
    self->m_wake.notify_all();
}

void VideoRenderer::Run() {
    std::string error;
    bool ok = SetUpEgl(error) && SetUpGl(error);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_startDone = true;
        m_startError = ok ? std::string() : error.empty() ? "OpenGL ES failed" : error;
        if (!ok) {
            m_stopping = true;
        }
    }
    m_wake.notify_all();
    if (!ok) {
        __android_log_print(ANDROID_LOG_WARN, kTag, "can't render: %s", error.c_str());
    }

    bool image = false;
    while (ok && Wait(image)) {
        if (image) {
            DrawLatest();
        }
        if (WithHands()) {
            DrawWithHands();
        } else if (!image) {
            RedrawIfResized();  // turned or resized: the last picture again, laid out anew
        }
    }
    TearDown();
}

bool VideoRenderer::Wait(bool& image) {
    int quarterTurns;
    float dotsPerInch;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        bool everyRefresh = WithHands() && !m_swapFailed;
        m_wake.wait_for(lock, everyRefresh ? kNoWait : kIdleCheck,
                        [this] { return m_stopping || m_imageReady; });
        if (m_stopping) {
            return false;
        }
        image = m_imageReady;
        m_imageReady = false;
        quarterTurns = m_quarterTurns;
        dotsPerInch = m_dotsPerInch;
    }
    if (m_hands) {
        m_hands->SetScreen(quarterTurns, dotsPerInch);
    }
    return true;
}

bool VideoRenderer::SetUpEgl(std::string& error) {
    m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (m_display == EGL_NO_DISPLAY || !eglInitialize(m_display, nullptr, nullptr)) {
        error = "no EGL display";
        return false;
    }
    // With a depth buffer, for the hands' parts to hide each other (hands/hand_renderer.h);
    // without one if there's none to be had.
    EGLint attributes[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                           EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
                           EGL_RED_SIZE,        8,
                           EGL_GREEN_SIZE,      8,
                           EGL_BLUE_SIZE,       8,
                           EGL_DEPTH_SIZE,      16,
                           EGL_NONE};
    EGLConfig config;
    EGLint configs = 0;
    if (!eglChooseConfig(m_display, attributes, &config, 1, &configs) || configs == 0) {
        attributes[std::size(attributes) - 3] = EGL_NONE;  // EGL_DEPTH_SIZE's place
        if (!eglChooseConfig(m_display, attributes, &config, 1, &configs) || configs == 0) {
            error = "no EGL config";
            return false;
        }
    }
    EGLint format = 0;
    eglGetConfigAttrib(m_display, config, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry(m_window, 0, 0, format);
    m_surface = eglCreateWindowSurface(m_display, config, m_window, nullptr);
    const EGLint contextAttributes[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    m_context = eglCreateContext(m_display, config, EGL_NO_CONTEXT, contextAttributes);
    if (m_surface == EGL_NO_SURFACE || m_context == EGL_NO_CONTEXT ||
        !eglMakeCurrent(m_display, m_surface, m_surface, m_context)) {
        error = "EGL surface or context failed (error " + std::to_string(eglGetError()) + ")";
        return false;
    }
    if (!m_procs.Load(m_display)) {
        error = "no EGL images from hardware buffers";
        return false;
    }
    m_textures = std::make_unique<HardwareBufferTextures>(m_display, m_procs, kMaxTextures);
    m_releaser = std::make_unique<ImageReleaser>(m_display, m_procs);
    return true;
}

bool VideoRenderer::SetUpGl(std::string& error) {
    const char* extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    if (!extensions || !strstr(extensions, "GL_OES_EGL_image_external")) {
        error = "no external textures";
        return false;
    }
    if (!m_program.Build(shaders::kVideoVertexShader, shaders::kVideoFragmentShader, error)) {
        return false;
    }
    m_program.Use();
    glUniform1i(m_program.Uniform("picture"), 0);
    m_lens = m_program.Uniform("lens");
    m_eyeLayout = m_program.Uniform("eyeLayout");
    m_crop = m_program.Uniform("crop");
    m_pairView = m_program.Uniform("pairView");
    m_bufferSize = m_program.Uniform("bufferSize");
    m_cursor = m_program.Uniform("cursor");
    m_pictureAspect = m_program.Uniform("pictureAspect");
    glVertexAttribPointer(ShaderProgram::kPositionAttribute, 2, GL_FLOAT, GL_FALSE, 0, kQuad);
    glEnableVertexAttribArray(ShaderProgram::kPositionAttribute);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    std::string handsError;
    m_hands = HandRenderer::Create(std::move(m_handsOptions), handsError);
    if (!m_hands) {
        __android_log_print(ANDROID_LOG_WARN, kTag, "no hands over the window: %s",
                            handsError.c_str());
    }
    return true;
}

void VideoRenderer::TearDown() {
    if (m_display == EGL_NO_DISPLAY) {
        return;
    }
    if (m_held) {
        glFinish();
        AImage_delete(m_held);
        m_held = nullptr;
    }
    m_hands.reset();  // its shader needs the context
    if (m_textures) {
        m_textures->Clear();
    }
    m_program.Release();
    eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (m_context != EGL_NO_CONTEXT) eglDestroyContext(m_display, m_context);
    if (m_surface != EGL_NO_SURFACE) eglDestroySurface(m_display, m_surface);
    // Not eglTerminate: the app's own UI draws with the same display.
    eglReleaseThread();
}

void VideoRenderer::DrawLatest() {
    AImage* image = nullptr;
    if (AImageReader_acquireLatestImage(m_reader, &image) != AMEDIA_OK || !image) {
        return;
    }
    AHardwareBuffer* buffer = nullptr;
    int64_t timestampNs = 0;
    Shown shown;
    AImage_getHardwareBuffer(image, &buffer);
    AImage_getTimestamp(image, &timestampNs);
    AImage_getCropRect(image, &shown.crop);
    shown.buffer = buffer;
    shown.view = ViewFor(timestampNs / 1000);  // the decoder stamps its pictures in ns
    const HardwareBufferTextures::Texture* texture = m_textures->Make(buffer);
    if (!texture) {
        AImage_delete(image);
        return;
    }
    m_shown = shown;
    if (ShowsHands(shown.view)) {
        m_hands->Enter();
        Hold(image);  // DrawWithHands draws it, every refresh
        return;
    }
    StopHands();
    Draw(*texture, shown);
    // The decoder may write the buffer again once the GPU is done reading it.
    m_releaser->Release(image);
    eglSwapBuffers(m_display, m_surface);
}

void VideoRenderer::RedrawIfResized() {
    EGLint width = 0, height = 0;
    eglQuerySurface(m_display, m_surface, EGL_WIDTH, &width);
    eglQuerySurface(m_display, m_surface, EGL_HEIGHT, &height);
    const HardwareBufferTextures::Texture* shown = m_textures->Find(m_shown.buffer);
    if (shown && (width != m_shownWidth || height != m_shownHeight)) {
        Draw(*shown, m_shown);
        eglSwapBuffers(m_display, m_surface);
    }
}

bool VideoRenderer::ShowsHands(const ViewInfo& view) const {
    return m_hands && view.app && (view.vr || m_handsDebug);
}

bool VideoRenderer::WithHands() const { return m_hands && m_hands->Active(); }

void VideoRenderer::DrawWithHands() {
    const HardwareBufferTextures::Texture* texture = m_textures->Find(m_shown.buffer);
    if (!texture) {
        return;
    }
    Draw(*texture, m_shown);
    m_hands->Draw(m_shown.view, m_shownWidth, m_shownHeight);
    m_swapFailed = eglSwapBuffers(m_display, m_surface) != EGL_TRUE;
}

void VideoRenderer::Hold(AImage* image) {
    m_releaser->Release(m_held);  // after the frames that drew it
    m_held = image;
}

void VideoRenderer::StopHands() {
    if (!WithHands()) {
        return;
    }
    m_releaser->Release(m_held);
    m_held = nullptr;
    m_hands->Leave();
    m_swapFailed = false;
}

void VideoRenderer::Draw(const HardwareBufferTextures::Texture& texture, const Shown& shown) {
    eglQuerySurface(m_display, m_surface, EGL_WIDTH, &m_shownWidth);
    eglQuerySurface(m_display, m_surface, EGL_HEIGHT, &m_shownHeight);
    const float width = float(m_shownWidth), height = float(m_shownHeight);
    glViewport(0, 0, m_shownWidth, m_shownHeight);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    if (texture.width == 0 || texture.height == 0 || width <= 0 || height <= 0) {
        return;
    }
    m_program.Use();  // the hands draw with their own
    // The hands point it at their mesh.
    glVertexAttribPointer(ShaderProgram::kPositionAttribute, 2, GL_FLOAT, GL_FALSE, 0, kQuad);

    const AImageCropRect& c = shown.crop;
    float pictureWidth = float(c.right - c.left), pictureHeight = float(c.bottom - c.top);
    if (pictureWidth <= 0 || pictureHeight <= 0) {
        pictureWidth = float(texture.width);
        pictureHeight = float(texture.height);
    }
    glUniform4f(m_crop, float(c.left) / texture.width, float(c.top) / texture.height,
                pictureWidth / texture.width, pictureHeight / texture.height);
    glUniform2f(m_bufferSize, float(texture.width), float(texture.height));
    // The cursor of the viewer's hand pointing at the window, where it points in the picture.
    const HandPointer::Cursor cursor = WithHands() ? m_hands->Cursor() : HandPointer::Cursor();
    glUniform4f(m_cursor, cursor.x, cursor.y, kCursorRadius,
                cursor.shown ? (cursor.pinched ? 2.0f : cursor.pinching) : -1.0f);
    glUniform1f(m_pictureAspect, pictureWidth / pictureHeight);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture.texture);

    const ViewInfo& view = shown.view;
    if (!view.vr) {
        // Whole, at its own shape, with black bars where the screen's differs, scaled
        // bicubically.
        float scale = std::min(width / pictureWidth, height / pictureHeight);
        GLsizei w = GLsizei(pictureWidth * scale), h = GLsizei(pictureHeight * scale);
        glViewport((m_shownWidth - w) / 2, (m_shownHeight - h) / 2, w, h);
        glUniform4f(m_eyeLayout, 0, 0, 0, 0);
        glUniform1f(m_pairView, -1);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        return;
    }
    // An eye per half of the screen, each with its lens center `separation` of the screen's
    // width from the other's.
    const GLsizei eyeWidth = m_shownWidth / 2;
    // A view of a pair is squeezed into half the width: unsqueezed, it has the frame's shape.
    const float pictureAspect = pictureWidth / pictureHeight;
    glUniform4f(m_lens, view.k1, view.k2, std::max(view.scale, 0.01f), view.chroma);
    for (int eye = 0; eye < 2; ++eye) {
        glViewport(eye * eyeWidth, 0, eyeWidth, m_shownHeight);
        float center = eye == 1 ? view.separation : 1 - view.separation;
        glUniform4f(m_eyeLayout, 1, center, float(eyeWidth) / height, pictureAspect);
        glUniform1f(m_pairView, view.pair ? float(eye) : -1.0f);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
}

}  // namespace gazeora
