#include "room/room_renderer.h"

#include <algorithm>

#include "room/room_geometry.h"
#include "shaders/room.frag.h"
#include "shaders/video.vert.h"

namespace gazeora {

namespace {

// Texture units: the stream, the camera, the 360 scene.
constexpr GLint kPictureUnit = 0;
constexpr GLint kCameraUnit = 1;
constexpr GLint kPanoramaUnit = 2;
// The shader's backdrop values.
constexpr float kBlack = 0;
constexpr float kCamera = 1;
constexpr float kScenery = 2;
// Below this many stream pixels per screen pixel, the window is sampled once.
constexpr float kMinifiedFrom = 1.0f;
constexpr float kTapSpread = 0.25f;

// A picture's place in its buffer, as the shaders take it: left, top, width, height (0..1).
void SetCrop(GLint uniform, const HardwareBufferTextures::Texture& texture,
             const AImageCropRect& c) {
    float width = float(c.right - c.left), height = float(c.bottom - c.top);
    if (width <= 0 || height <= 0) {
        glUniform4f(uniform, 0, 0, 1, 1);
        return;
    }
    glUniform4f(uniform, float(c.left) / texture.width, float(c.top) / texture.height,
                width / texture.width, height / texture.height);
}

}  // namespace

std::unique_ptr<RoomRenderer> RoomRenderer::Create(EGLDisplay display, const EglProcs& procs,
                                                   const ImageReleaser& releaser,
                                                   std::string& error) {
    std::unique_ptr<RoomRenderer> room(new RoomRenderer(display, procs, releaser));
    if (!room->Build(error)) {
        room.reset();
    }
    return room;
}

RoomRenderer::RoomRenderer(EGLDisplay display, const EglProcs& procs, const ImageReleaser& releaser)
    : m_camera(display, procs, releaser) {}

bool RoomRenderer::Build(std::string& error) {
    if (!m_program.Build(shaders::kVideoVertexShader, shaders::kRoomFragmentShader, error)) {
        return false;
    }
    m_program.Use();
    glUniform1i(m_program.Uniform("picture"), kPictureUnit);
    glUniform1i(m_program.Uniform("camera"), kCameraUnit);
    glUniform1i(m_program.Uniform("panorama"), kPanoramaUnit);
    Uniforms& u = m_uniforms;
    u.lens = m_program.Uniform("lens");
    u.eyeLayout = m_program.Uniform("eyeLayout");
    u.pixelTan = m_program.Uniform("pixelTan");
    u.crop = m_program.Uniform("crop");
    u.pictureTap = m_program.Uniform("pictureTap");
    u.screenToRoom = m_program.Uniform("screenToRoom");
    u.eye = m_program.Uniform("eye");
    u.panel = m_program.Uniform("panel");
    u.cameraCrop = m_program.Uniform("cameraCrop");
    u.viewToSensor = m_program.Uniform("viewToSensor");
    u.cameraTan = m_program.Uniform("cameraTan");
    u.backdrop = m_program.Uniform("backdrop");
    return true;
}

void RoomRenderer::Enter() {
    if (m_active) {
        return;
    }
    m_active = true;
    m_anchor.Clear();  // placed ahead with the first reading
    m_head.Start();
    m_camera.Open();
}

void RoomRenderer::Leave() {
    if (!m_active) {
        return;
    }
    m_active = false;
    m_head.Stop();
    m_camera.Close();
}

void RoomRenderer::SetScreen(int quarterTurns, float dotsPerInch) {
    m_quarterTurns = quarterTurns;
    m_dotsPerInch = dotsPerInch;
}

bool RoomRenderer::TakePanoramaRequest() {
    return m_active && m_camera.Failed() && m_panorama.TakeRequest();
}

void RoomRenderer::Draw(const HardwareBufferTextures::Texture& picture, const AImageCropRect& crop,
                        const ViewInfo& view, EGLint width, EGLint height) {
    glViewport(0, 0, width, height);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    if (picture.width == 0 || picture.height == 0 || width <= 0 || height <= 0) {
        return;
    }
    m_program.Use();
    const Mat3 head = HeadNow();
    float columns[9];
    head.ToColumnMajor(columns);
    glUniformMatrix3fv(m_uniforms.screenToRoom, 1, GL_FALSE, columns);
    glUniform1f(m_uniforms.backdrop, BindBackdrop());

    const float tanPerUnit = room_geometry::TanPerUnit(height, m_dotsPerInch);
    SetWindow(picture, crop, tanPerUnit, height);
    glUniform4f(m_uniforms.lens, view.k1, view.k2, 0, view.chroma);
    const EGLint eyeWidth = width / 2;
    for (int eye = 0; eye < 2; ++eye) {
        glViewport(eye * eyeWidth, 0, eyeWidth, height);
        float center = eye == 1 ? view.separation : 1 - view.separation;
        glUniform4f(m_uniforms.eyeLayout, 1, center, float(eyeWidth) / height, tanPerUnit);
        float side = eye == 1 ? 0.5f : -0.5f;
        Vec3 position = head.Apply({side * room_geometry::kEyeSeparation, 0, 0});
        glUniform3f(m_uniforms.eye, position.x, position.y, position.z);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
}

Mat3 RoomRenderer::HeadNow() {
    Mat3 screenToWorld;
    if (!m_head.Read(m_quarterTurns, screenToWorld)) {
        return m_anchor.ScreenToRoom(Mat3());  // no reading (yet): the window stays ahead
    }
    if (!m_anchor.Placed()) {
        m_anchor.Place(screenToWorld);
    }
    return m_anchor.ScreenToRoom(screenToWorld);
}

float RoomRenderer::BindBackdrop() {
    AImageCropRect crop = {};
    const HardwareBufferTextures::Texture* camera = m_camera.Latest(crop);
    if (camera) {
        glActiveTexture(GL_TEXTURE0 + kCameraUnit);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, camera->texture);
        SetCrop(m_uniforms.cameraCrop, *camera, crop);
        const CameraSetup& setup = m_camera.Setup();
        CameraView view = setup.view;
        camera_geometry::SetOrientation(view, setup.sensorOrientation, m_quarterTurns);
        const GLfloat toSensor[4] = {float(view.toSensor[0][0]), float(view.toSensor[1][0]),
                                     float(view.toSensor[0][1]), float(view.toSensor[1][1])};
        glUniformMatrix2fv(m_uniforms.viewToSensor, 1, GL_FALSE, toSensor);
        glUniform2f(m_uniforms.cameraTan, view.tanAcross, view.tanDown);
        return kCamera;
    }
    if (m_camera.Failed()) {
        glActiveTexture(GL_TEXTURE0 + kPanoramaUnit);
        if (m_panorama.Bind()) return kScenery;
    }
    return kBlack;  // the camera is still opening
}

void RoomRenderer::SetWindow(const HardwareBufferTextures::Texture& picture,
                             const AImageCropRect& crop, float tanPerUnit, EGLint height) {
    glActiveTexture(GL_TEXTURE0 + kPictureUnit);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, picture.texture);
    SetCrop(m_uniforms.crop, picture, crop);
    float pictureWidth = float(crop.right - crop.left),
          pictureHeight = float(crop.bottom - crop.top);
    if (pictureWidth <= 0 || pictureHeight <= 0) {
        pictureWidth = float(picture.width);
        pictureHeight = float(picture.height);
    }
    const float halfWidth = room_geometry::kWindowWidth / 2;
    glUniform4f(m_uniforms.panel, halfWidth, halfWidth * pictureHeight / pictureWidth,
                room_geometry::kWindowDistance, room_geometry::kWindowCornerRadius);

    // Smaller on the screen than in the stream, the window is sampled four times a pixel, so
    // its text doesn't shimmer as the head moves.
    const float pixelTan = tanPerUnit / (height / 2.0f);
    glUniform1f(m_uniforms.pixelTan, pixelTan);
    float onScreen = 2 * halfWidth / room_geometry::kWindowDistance / pixelTan;  // pixels
    float perPixel = pictureWidth / std::max(onScreen, 1.0f);
    float tap = perPixel > kMinifiedFrom ? kTapSpread * perPixel : 0;
    glUniform2f(m_uniforms.pictureTap, tap / picture.width, tap / picture.height);
}

}  // namespace gazeora
