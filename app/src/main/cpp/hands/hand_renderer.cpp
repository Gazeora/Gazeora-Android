#include "hands/hand_renderer.h"

#include <android/log.h>

#include <algorithm>
#include <chrono>
#include <cmath>

#include "hands/hand_skeleton.h"
#include "hands/view_geometry.h"
#include "shaders/hand.frag.h"
#include "shaders/hand.vert.h"

namespace gazeora {

namespace {

// How long the drawn light takes to settle on a change in the room's, in seconds: the
// camera's own exposure wavers.
constexpr double kLightSettles = 0.5;
constexpr char kTag[] = "GazeoraHands";
// The outline's width round the hands, as a tangent: about a millimeter at arm's length.
constexpr float kOutlineWidth = 0.0035f;
// The hands are drawn this much smaller than the viewer's, about the middle of the palm, so
// they don't hide as much of the window.
constexpr float kDrawnSize = 0.75f;

// Without a depth buffer: the farthest of `corners`' triangles first, so the nearer ones go
// over them.
void SortFarFirst(const std::vector<HandVertex>& corners, const std::vector<uint16_t>& triangles,
                  std::vector<uint16_t>& order) {
    struct Triangle {
        uint16_t corners[3];
    };
    order = triangles;
    auto* first = reinterpret_cast<Triangle*>(order.data());
    auto depth = [&](const Triangle& t) {
        return corners[t.corners[0]].z + corners[t.corners[1]].z + corners[t.corners[2]].z;
    };
    std::sort(first, first + order.size() / 3,
              [&](const Triangle& a, const Triangle& b) { return depth(a) > depth(b); });
}

double NowSeconds() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

}  // namespace

std::unique_ptr<HandRenderer> HandRenderer::Create(HandsOptions options, std::string& error) {
    std::unique_ptr<HandRenderer> hands(new HandRenderer(std::move(options)));
    if (!hands->Build(error)) {
        hands.reset();
    }
    return hands;
}

HandRenderer::HandRenderer(HandsOptions options)
    : m_pointer(std::move(options.pointer)),
      m_tracker(std::move(options.models)),
      m_camera([this](const YuvPlanes& yuv, const CameraView& view, int64_t timestampNs) {
          m_tracker.Track(yuv, view, timestampNs);
      }),
      m_making(std::async(std::launch::async, [] { return &RiggedHand::Get(); })) {}

bool HandRenderer::Build(std::string& error) {
    if (!m_program.Build(shaders::kHandVertexShader, shaders::kHandFragmentShader, error)) {
        return false;
    }
    Uniforms& u = m_uniforms;
    u.lens = m_program.Uniform("lens");
    u.eyeLayout = m_program.Uniform("eyeLayout");
    u.lightFrom = m_program.Uniform("lightFrom");
    u.lightColor = m_program.Uniform("lightColor");
    u.ambient = m_program.Uniform("ambient");
    u.outline = m_program.Uniform("outline");
    Attributes& a = m_attributes;
    a.normal = m_program.Attribute("normal");
    a.shows = m_program.Attribute("shows");
    a.glow = m_program.Attribute("glow");
    if (a.normal < 0 || a.shows < 0 || a.glow < 0) {
        error = "the hand shader lost its attributes";
        return false;
    }
    GLint depthBits = 0;
    glGetIntegerv(GL_DEPTH_BITS, &depthBits);
    m_depthTest = depthBits > 0;
    return true;
}

void HandRenderer::Enter() {
    if (m_active) {
        return;
    }
    m_active = true;
    m_tracker.Start();
    m_camera.Open();
}

void HandRenderer::Leave() {
    if (!m_active) {
        return;
    }
    m_active = false;
    m_camera.Close();
    m_tracker.Stop();
    m_hands.Clear();
    m_shapes.Clear();
}

void HandRenderer::SetScreen(int quarterTurns, float dotsPerInch) {
    m_quarterTurns = quarterTurns;
    m_dotsPerInch = dotsPerInch;
    m_camera.SetQuarterTurns(quarterTurns);
}

void HandRenderer::Draw(const ViewInfo& view, EGLint width, EGLint height) {
    m_cursor = {};
    m_camera.Poll();
    if (!m_camera.Running() || width <= 0 || height <= 0) {
        return;
    }
    // The hands show the way the camera sees them, which looks out from the viewer's head as
    // their eyes do.
    const CameraSetup& setup = m_camera.Setup();
    CameraView camera = setup.view;
    camera_geometry::SetOrientation(camera, setup.sensorOrientation, m_quarterTurns);
    std::vector<HandLandmarks> seen;
    if (m_tracker.Take(seen)) {
        m_hands.Set(std::move(seen));
    }
    const std::vector<HandSkeleton> hands = m_hands.Hands(camera);
    for (const PointerEvent& event : m_hands.TakePointerEvents()) {
        if (m_pointer) m_pointer(event);
    }
    m_cursor = m_hands.Cursor();
    // The light moves on, hands or not, so they show in the room's light when they come.
    const double now = NowSeconds();
    const float settled = m_lightAt > 0 ? float(1 - std::exp(-(now - m_lightAt) / kLightSettles))
                                        : 1.0f;
    m_light = light_estimate::Blend(m_light, m_camera.Light(), settled);
    m_lightAt = now;
    if (!m_rig) {
        if (m_making.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
            return;
        }
        m_rig = m_making.get();
    }
    Pose(hands, now);
    // For a flat picture, one eye as wide as the screen, without a lens, as far up and down as
    // the camera sees.
    const bool flat = !view.vr;
    const int eyes = flat ? 1 : 2;
    const EGLint eyeWidth = width / eyes;
    // With the cursor of a hand pointing at the window, it's the hand: none drawn over it.
    if (hands.empty() || m_cursor.shown) {
        return;
    }

    m_program.Use();
    float tanAcross = 0, tanDown = 0;
    camera_geometry::UprightTangents(camera, tanAcross, tanDown);
    const float tanPerUnit = flat ? tanDown : eye_geometry::TanPerUnit(height, m_dotsPerInch);
    glUniform4f(m_uniforms.lens, flat ? 0 : view.k1, flat ? 0 : view.k2, 0, 0);
    glUniform3f(m_uniforms.lightFrom, m_light.from.x, m_light.from.y, m_light.from.z);
    glUniform3f(m_uniforms.lightColor, m_light.color[0] * m_light.strength,
                m_light.color[1] * m_light.strength, m_light.color[2] * m_light.strength);
    glUniform1f(m_uniforms.ambient, m_light.ambient);
    const GLint attributes[] = {m_attributes.normal, m_attributes.shows, m_attributes.glow};
    for (GLint attribute : attributes) {
        glEnableVertexAttribArray(attribute);
    }
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (m_depthTest) {
        glViewport(0, 0, width, height);
        glDepthMask(GL_TRUE);
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
    }
    for (int eye = 0; eye < eyes; ++eye) {
        glViewport(eye * eyeWidth, 0, eyeWidth, height);
        // Both eyes' lens centers sit the same way off their middles, mirrored.
        float center = flat ? 0.5f : eye == 1 ? view.separation : 1 - view.separation;
        glUniform4f(m_uniforms.eyeLayout, 1, center, float(eyeWidth) / height, tanPerUnit);
        DrawHands();
    }
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    // The window's shader has only `position`, which the window points back at its quad.
    for (GLint attribute : attributes) {
        glDisableVertexAttribArray(attribute);
    }
}

void HandRenderer::Pose(const std::vector<HandSkeleton>& skeletons, double now) {
    for (const HandShapeChange& change : m_shapes.Update(skeletons, now)) {
        __android_log_print(ANDROID_LOG_DEBUG, kTag, "%s hand %s %s",
                            change.right ? "right" : "left", hand_shapes::Name(change.shape),
                            change.began ? "began" : "ended");
    }
    m_posed.resize(skeletons.size());
    m_order.resize(skeletons.size());
    for (size_t h = 0; h < skeletons.size(); ++h) {
        // The thumb's and index finger's tips glow as they near a pinch: the last two bones of
        // each (RiggedHand's numbering: the palm, then each finger's three).
        float glow[RiggedHand::kBones] = {};
        const float pinching = hand_shapes::Pinching(skeletons[h]);
        for (int f = 0; f < 2; ++f) {
            for (int b = 1; b < HandSkeleton::kBones; ++b) {
                glow[1 + f * HandSkeleton::kBones + b] = pinching;
            }
        }
        m_rig->Pose(skeletons[h], m_posed[h], glow);
        const HandSkeleton& hand = skeletons[h];
        const Vec3 middle = (hand.wrist + hand.bones[2][0].start) * 0.5f;
        for (HandVertex& corner : m_posed[h]) {
            corner.x = middle.x + (corner.x - middle.x) * kDrawnSize;
            corner.y = middle.y + (corner.y - middle.y) * kDrawnSize;
            corner.z = middle.z + (corner.z - middle.z) * kDrawnSize;
        }
        if (!m_depthTest) {
            SortFarFirst(m_posed[h], m_rig->Triangles(), m_order[h]);
        }
    }
}

void HandRenderer::DrawHands() {
    for (size_t h = 0; h < m_posed.size(); ++h) {
        const HandVertex* corners = m_posed[h].data();
        const std::vector<uint16_t>& triangles = m_depthTest ? m_rig->Triangles() : m_order[h];
        const GLsizei stride = sizeof(HandVertex);
        glVertexAttribPointer(ShaderProgram::kPositionAttribute, 3, GL_FLOAT, GL_FALSE, stride,
                              &corners->x);
        glVertexAttribPointer(m_attributes.normal, 3, GL_FLOAT, GL_FALSE, stride, &corners->nx);
        glVertexAttribPointer(m_attributes.shows, 1, GL_FLOAT, GL_FALSE, stride,
                              &corners->shows);
        glVertexAttribPointer(m_attributes.glow, 1, GL_FLOAT, GL_FALSE, stride, &corners->glow);
        auto draw = [&] {
            glDrawElements(GL_TRIANGLES, GLsizei(triangles.size()), GL_UNSIGNED_SHORT,
                           triangles.data());
        };
        glUniform1f(m_uniforms.outline, 0);
        if (m_depthTest) {
            // Its nearest surface's depth first, so it's colored once however it folds; then
            // the outline, behind it, shows only past its edge.
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LESS);
            draw();
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glDepthMask(GL_FALSE);
            glDepthFunc(GL_LEQUAL);
        }
        glUniform1f(m_uniforms.outline, kOutlineWidth);
        draw();
        glUniform1f(m_uniforms.outline, 0);
        draw();
    }
}

}  // namespace gazeora
