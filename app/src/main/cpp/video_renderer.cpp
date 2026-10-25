#include "video_renderer.h"

#include <android/log.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstring>

namespace gazeora {

namespace {

constexpr char kTag[] = "GazeoraRenderer";
// The decoder's pictures in flight: one it's drawing, one queued, one being shown.
constexpr int32_t kReaderImages = 4;
// Buffers whose textures are kept; the decoder cycles through fewer.
constexpr size_t kMaxTextures = 8;
// How often to look for a change in the window's size while no pictures come.
constexpr auto kIdleCheck = std::chrono::milliseconds(100);
// View infos waiting for their picture; more means the pictures went elsewhere.
constexpr size_t kMaxViews = 64;

const char kVertexShader[] = R"(
attribute vec2 position;
varying vec2 uv;
void main() {
    uv = vec2(position.x * 0.5 + 0.5, 0.5 - position.y * 0.5);  // 0, 0 at the top left
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

// The PC's warp (frame_dump.cpp in Gazeora), per eye: each pixel measured from its lens center
// in units of half the eye's height, pushed outwards by the distortion polynomial, then looked
// up in the picture; red and blue a little smaller and larger against the lenses' color
// fringes. In a pair of views, each eye looks in its own half.
const char kFragmentShader[] = R"(
#extension GL_OES_EGL_image_external : require
precision highp float;
uniform samplerExternalOES picture;
uniform vec4 lens;    // k1, k2, picture scale, chroma correction
// Not "layout": Adreno drivers (Redmi Y3) reject it as a keyword, and VR then can't draw.
uniform vec4 eyeLayout;  // VR (0/1), the lens center's x in the eye, eye aspect, picture aspect
uniform vec4 crop;    // the picture in the buffer: left, top, width, height (0..1)
uniform float pairView;  // the view of a pair this eye shows (0 left, 1 right); -1 no pair
uniform vec2 bufferSize;  // in texels
varying vec2 uv;

// Catmull-Rom bicubic in 9 bilinear taps (the middle two of each row and column merged into
// one). Sharper than bilinear when the picture is scaled to the screen, and exact at 1:1: a
// screen pixel on a texel's center takes that texel. Taps stay inside the picture, as the
// buffer may have padding beyond it.
vec3 Bicubic(vec2 at) {
    vec2 position = at * bufferSize;
    vec2 center1 = floor(position - 0.5) + 0.5;
    vec2 f = position - center1;
    vec2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
    vec2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
    vec2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));
    vec2 w3 = f * f * (-0.5 + 0.5 * f);
    vec2 w12 = w1 + w2;
    vec2 low = crop.xy + 0.5 / bufferSize, high = crop.xy + crop.zw - 0.5 / bufferSize;
    vec2 p0 = clamp((center1 - 1.0) / bufferSize, low, high);
    vec2 p12 = clamp((center1 + w2 / w12) / bufferSize, low, high);
    vec2 p3 = clamp((center1 + 2.0) / bufferSize, low, high);
    vec3 color = (texture2D(picture, vec2(p0.x, p0.y)).rgb * w0.x +
                  texture2D(picture, vec2(p12.x, p0.y)).rgb * w12.x +
                  texture2D(picture, vec2(p3.x, p0.y)).rgb * w3.x) * w0.y;
    color += (texture2D(picture, vec2(p0.x, p12.y)).rgb * w0.x +
              texture2D(picture, vec2(p12.x, p12.y)).rgb * w12.x +
              texture2D(picture, vec2(p3.x, p12.y)).rgb * w3.x) * w12.y;
    color += (texture2D(picture, vec2(p0.x, p3.y)).rgb * w0.x +
              texture2D(picture, vec2(p12.x, p3.y)).rgb * w12.x +
              texture2D(picture, vec2(p3.x, p3.y)).rgb * w3.x) * w3.y;
    return clamp(color, 0.0, 1.0);
}

vec2 InBuffer(vec2 q) {
    if (pairView >= 0.0) q.x = (q.x + pairView) * 0.5;
    return crop.xy + q * crop.zw;
}
vec2 ThroughLens(vec2 q, float channelScale) {
    return 0.5 + q * channelScale / (2.0 * lens.z) * vec2(1.0 / eyeLayout.w, 1.0);
}
float Channel(vec2 q, int channel) {
    if (q.x < 0.0 || q.y < 0.0 || q.x > 1.0 || q.y > 1.0) return 0.0;
    vec3 color = texture2D(picture, InBuffer(q)).rgb;
    return channel == 0 ? color.r : channel == 1 ? color.g : color.b;
}
void main() {
    if (eyeLayout.x == 0.0) {
        gl_FragColor = vec4(Bicubic(InBuffer(uv)), 1.0);
        return;
    }
    vec2 p = (uv - vec2(eyeLayout.y, 0.5)) * vec2(eyeLayout.z, 1.0) * 2.0;
    float r2 = dot(p, p);
    vec2 q = p * (1.0 + lens.x * r2 + lens.y * r2 * r2);
    gl_FragColor = vec4(Channel(ThroughLens(q, 1.0 - lens.w), 0),
                        Channel(ThroughLens(q, 1.0), 1),
                        Channel(ThroughLens(q, 1.0 + lens.w), 2), 1.0);
}
)";

const GLfloat kQuad[] = {-1, -1, 1, -1, -1, 1, 1, 1};

GLuint Compile(GLenum type, const char* source, std::string& error) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512] = {};
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        error = std::string("shader: ") + log;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

template <typename Fn>
Fn Proc(const char* name) {
    return reinterpret_cast<Fn>(eglGetProcAddress(name));
}

}  // namespace

VideoRenderer::VideoRenderer(ANativeWindow* window) : m_window(window) {}

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

    while (ok) {
        bool image;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_wake.wait_for(lock, kIdleCheck, [this] { return m_stopping || m_imageReady; });
            if (m_stopping) {
                break;
            }
            image = m_imageReady;
            m_imageReady = false;
        }
        if (image) {
            DrawLatest();
            continue;
        }
        // Turned or resized: the last picture again, laid out for the new shape.
        EGLint width = 0, height = 0;
        eglQuerySurface(m_display, m_surface, EGL_WIDTH, &width);
        eglQuerySurface(m_display, m_surface, EGL_HEIGHT, &height);
        const Texture* shown = TextureFor(m_shown.buffer, false);
        if (shown && (width != m_shownWidth || height != m_shownHeight)) {
            Draw(*shown, m_shown);
            eglSwapBuffers(m_display, m_surface);
        }
    }
    TearDown();
}

bool VideoRenderer::SetUpEgl(std::string& error) {
    m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (m_display == EGL_NO_DISPLAY || !eglInitialize(m_display, nullptr, nullptr)) {
        error = "no EGL display";
        return false;
    }
    const EGLint attributes[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                                 EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
                                 EGL_RED_SIZE,        8,
                                 EGL_GREEN_SIZE,      8,
                                 EGL_BLUE_SIZE,       8,
                                 EGL_NONE};
    EGLConfig config;
    EGLint configs = 0;
    if (!eglChooseConfig(m_display, attributes, &config, 1, &configs) || configs == 0) {
        error = "no EGL config";
        return false;
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

    m_getNativeClientBuffer =
        Proc<PFNEGLGETNATIVECLIENTBUFFERANDROIDPROC>("eglGetNativeClientBufferANDROID");
    m_createImage = Proc<PFNEGLCREATEIMAGEKHRPROC>("eglCreateImageKHR");
    m_destroyImage = Proc<PFNEGLDESTROYIMAGEKHRPROC>("eglDestroyImageKHR");
    m_imageTargetTexture =
        Proc<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>("glEGLImageTargetTexture2DOES");
    const char* extensions = eglQueryString(m_display, EGL_EXTENSIONS);
    if (extensions && strstr(extensions, "EGL_ANDROID_native_fence_sync")) {
        m_createSync = Proc<PFNEGLCREATESYNCKHRPROC>("eglCreateSyncKHR");
        m_destroySync = Proc<PFNEGLDESTROYSYNCKHRPROC>("eglDestroySyncKHR");
        m_dupFence = Proc<PFNEGLDUPNATIVEFENCEFDANDROIDPROC>("eglDupNativeFenceFDANDROID");
    }
    if (!m_getNativeClientBuffer || !m_createImage || !m_destroyImage || !m_imageTargetTexture) {
        error = "no EGL images from hardware buffers";
        return false;
    }
    return true;
}

bool VideoRenderer::SetUpGl(std::string& error) {
    const char* extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    if (!extensions || !strstr(extensions, "GL_OES_EGL_image_external")) {
        error = "no external textures";
        return false;
    }
    GLuint vertex = Compile(GL_VERTEX_SHADER, kVertexShader, error);
    GLuint fragment = vertex ? Compile(GL_FRAGMENT_SHADER, kFragmentShader, error) : 0;
    if (!fragment) {
        if (vertex) glDeleteShader(vertex);
        return false;
    }
    m_program = glCreateProgram();
    glAttachShader(m_program, vertex);
    glAttachShader(m_program, fragment);
    glBindAttribLocation(m_program, 0, "position");
    glLinkProgram(m_program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    GLint linked = 0;
    glGetProgramiv(m_program, GL_LINK_STATUS, &linked);
    if (!linked) {
        error = "shader program didn't link";
        return false;
    }
    glUseProgram(m_program);
    glUniform1i(glGetUniformLocation(m_program, "picture"), 0);
    m_lens = glGetUniformLocation(m_program, "lens");
    m_eyeLayout = glGetUniformLocation(m_program, "eyeLayout");
    m_crop = glGetUniformLocation(m_program, "crop");
    m_pairView = glGetUniformLocation(m_program, "pairView");
    m_bufferSize = glGetUniformLocation(m_program, "bufferSize");
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, kQuad);
    glEnableVertexAttribArray(0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    return true;
}

void VideoRenderer::TearDown() {
    if (m_display == EGL_NO_DISPLAY) {
        return;
    }
    for (Texture& texture : m_textures) {
        glDeleteTextures(1, &texture.texture);
        m_destroyImage(m_display, texture.image);
        AHardwareBuffer_release(texture.buffer);
    }
    m_textures.clear();
    if (m_program) {
        glDeleteProgram(m_program);
    }
    eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (m_context != EGL_NO_CONTEXT) eglDestroyContext(m_display, m_context);
    if (m_surface != EGL_NO_SURFACE) eglDestroySurface(m_display, m_surface);
    // Not eglTerminate: the app's own UI draws with the same display.
    eglReleaseThread();
}

const VideoRenderer::Texture* VideoRenderer::TextureFor(AHardwareBuffer* buffer, bool make) {
    for (const Texture& texture : m_textures) {
        if (texture.buffer == buffer) return &texture;
    }
    if (!make || !buffer) {
        return nullptr;
    }
    EGLClientBuffer client = m_getNativeClientBuffer(buffer);
    const EGLint attributes[] = {EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE};
    EGLImageKHR image = client ? m_createImage(m_display, EGL_NO_CONTEXT,
                                               EGL_NATIVE_BUFFER_ANDROID, client, attributes)
                               : EGL_NO_IMAGE_KHR;
    if (image == EGL_NO_IMAGE_KHR) {
        return nullptr;
    }
    if (m_textures.size() >= kMaxTextures) {
        Texture& oldest = m_textures.front();
        glDeleteTextures(1, &oldest.texture);
        m_destroyImage(m_display, oldest.image);
        AHardwareBuffer_release(oldest.buffer);
        m_textures.erase(m_textures.begin());
    }
    Texture texture;
    AHardwareBuffer_acquire(buffer);  // so a buffer seen before is always the same one
    texture.buffer = buffer;
    texture.image = image;
    AHardwareBuffer_Desc desc = {};
    AHardwareBuffer_describe(buffer, &desc);
    texture.width = desc.width;
    texture.height = desc.height;
    glGenTextures(1, &texture.texture);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture.texture);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_imageTargetTexture(GL_TEXTURE_EXTERNAL_OES, static_cast<GLeglImageOES>(image));
    m_textures.push_back(texture);
    return &m_textures.back();
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
    const Texture* texture = TextureFor(buffer, true);
    if (!texture) {
        AImage_delete(image);
        return;
    }
    Draw(*texture, shown);
    m_shown = shown;

    // The decoder may write the buffer again once the GPU is done reading it.
    EGLSyncKHR sync = m_createSync ? m_createSync(m_display, EGL_SYNC_NATIVE_FENCE_ANDROID, nullptr)
                                   : EGL_NO_SYNC_KHR;
    eglSwapBuffers(m_display, m_surface);
    int fence = sync != EGL_NO_SYNC_KHR ? m_dupFence(m_display, sync) : -1;
    if (sync != EGL_NO_SYNC_KHR) {
        m_destroySync(m_display, sync);
    }
    if (fence >= 0) {
        AImage_deleteAsync(image, fence);
    } else {
        glFinish();
        AImage_delete(image);
    }
}

void VideoRenderer::Draw(const Texture& texture, const Shown& shown) {
    eglQuerySurface(m_display, m_surface, EGL_WIDTH, &m_shownWidth);
    eglQuerySurface(m_display, m_surface, EGL_HEIGHT, &m_shownHeight);
    const float width = float(m_shownWidth), height = float(m_shownHeight);
    glViewport(0, 0, m_shownWidth, m_shownHeight);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    if (texture.width == 0 || texture.height == 0 || width <= 0 || height <= 0) {
        return;
    }

    const AImageCropRect& c = shown.crop;
    float pictureWidth = float(c.right - c.left), pictureHeight = float(c.bottom - c.top);
    if (pictureWidth <= 0 || pictureHeight <= 0) {
        pictureWidth = float(texture.width);
        pictureHeight = float(texture.height);
    }
    glUniform4f(m_crop, float(c.left) / texture.width, float(c.top) / texture.height,
                pictureWidth / texture.width, pictureHeight / texture.height);
    glUniform2f(m_bufferSize, float(texture.width), float(texture.height));
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
