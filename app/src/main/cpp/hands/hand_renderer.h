// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <EGL/egl.h>

#include <functional>
#include <future>
#include <memory>
#include <string>
#include <vector>

#include "hands/camera_feed.h"
#include "hands/camera_light_gate.h"
#include "hands/camera_status_reporter.h"
#include "hands/hand_overlay.h"
#include "hands/hand_shapes.h"
#include "hands/hand_tracker.h"
#include "hands/light_estimate.h"
#include "hands/rigged_hand.h"
#include "protocols/view_info.h"
#include "shaders/shader_program.h"

namespace gazeora {

// How the viewer's hands are set up.
struct HandsOptions {
    HandModelFiles models;
    // For debugging (debuggable builds only): the hands over a flat picture of Gazeora's window
    // too.
    bool debug = false;
    // Where what the viewer's hand pointing at the window has the PC do goes; from the render
    // thread.
    std::function<void(const PointerEvent&)> pointer;
    // Where the camera's status goes, for the PC's overlay; from the render thread.
    CameraStatusReporter::Send cameraStatus;
};

// Facade over the viewer's hands in a VR viewer, as VR headsets show them: the back camera
// looks for them, unseen, and where the hand tracker (hand_tracker.h) finds the viewer's own,
// rigged hands on a
// skeleton that only bends as a hand can follow them over Gazeora's window, in both eyes
// through the lenses, shaded a little by the room's own light as the camera sees it. Made, used and
// destroyed on the render thread, with its context current.
class HandRenderer {
public:
    // Null with `error` where the hands can't be drawn.
    static std::unique_ptr<HandRenderer> Create(HandsOptions options, std::string& error);
    HandRenderer(const HandRenderer&) = delete;
    HandRenderer& operator=(const HandRenderer&) = delete;

    // Gazeora's window appeared: the camera on, while there's light enough to see hands.
    void Enter();
    // Back to games: the camera off.
    void Leave();
    bool Active() const { return m_active; }

    void SetScreen(int quarterTurns, float dotsPerInch);
    // Where to draw the cursor of the viewer's hand pointing at the window, as of the last
    // Draw.
    const HandPointer::Cursor& Cursor() const { return m_cursor; }

    // Over what both eyes of the surface, `width` x `height`, show already, through the lenses
    // `view` says; or, for a flat picture (when debugging), over the whole surface, as far
    // across it as the camera sees.
    void Draw(const ViewInfo& view, EGLint width, EGLint height);

private:
    struct Uniforms {
        GLint lens = -1;
        GLint eyeLayout = -1;
        GLint lightFrom = -1;
        GLint lightColor = -1;
        GLint ambient = -1;
        GLint outline = -1;
    };
    struct Attributes {
        GLint normal = -1;
        GLint shows = -1;
        GLint glow = -1;
    };

    explicit HandRenderer(HandsOptions options);
    bool Build(std::string& error);
    // Poses the hands anew, saying what shapes they begin and stop making.
    void Pose(const std::vector<HandSkeleton>& hands, double now);
    // Each hand's triangles, its outline then itself, through the eye set up already.
    void DrawHands();
    // Turns the camera on or off as the light gate says, and reports how it stands.
    void PowerCamera();

    ShaderProgram m_program;
    Uniforms m_uniforms;
    Attributes m_attributes;
    // Whether the surface has a depth buffer, so only a hand's nearest surface shows; else its
    // triangles are drawn farthest first.
    bool m_depthTest = false;
    std::function<void(const PointerEvent&)> m_pointer;
    HandPointer::Cursor m_cursor;
    HandTracker m_tracker;
    CameraFeed m_camera;  // after the tracker, which it gives its pictures
    CameraLightGate m_gate;
    CameraStatusReporter m_cameraStatus;
    HandOverlay m_hands;
    // The rigged hand, made on its own thread at first: no hands till it's ready.
    std::future<const RiggedHand*> m_making;
    const RiggedHand* m_rig = nullptr;
    // The hands posed, every refresh, as they glide. Without a depth buffer, each one's
    // triangles in its own order.
    std::vector<std::vector<HandVertex>> m_posed;
    std::vector<std::vector<uint16_t>> m_order;
    HandShapeWatcher m_shapes;
    // The light they're drawn in, moving smoothly to the room's.
    SceneLight m_light;
    double m_lightAt = 0;
    bool m_active = false;
    int m_quarterTurns = 1;
    float m_dotsPerInch = 0;
};

}  // namespace gazeora
