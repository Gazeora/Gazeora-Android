// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <android/native_window.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "hands/camera_status_sender.h"
#include "head/head_tracker.h"
#include "links/message_link.h"
#include "pointer/pointer_sender.h"
#include "sources/stream_source.h"
#include "video_decoder.h"
#include "video_renderer.h"

namespace gazeora {

// Plays the PC's stream onto a window, over whichever protocol Gazeora streams with: a source
// for the protocol, the decoder, and the renderer that lays each picture out (for a VR viewer
// when the PC says so). Where OpenGL ES won't run, the decoder draws straight to the window,
// as it is. Touches, and while the picture is shown for a viewer the head's turns, go back to
// the PC.
class Player : private SourceSink, private VideoDecoder::Listener {
public:
    // From the player's threads.
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void OnStatus(PlayerStatus status, const std::string& detail) = 0;
        // The picture's size when the window should take its shape; 0 x 0 when the window
        // should fill the screen, as the renderer fits the picture in itself.
        virtual void OnVideoSize(int width, int height) = 0;
        virtual void OnFirstFrame() = 0;
        // How the picture is shown: for a VR viewer or flat, and its size; again whenever
        // either changes. Touches only make sense on a flat one.
        virtual void OnPicture(bool vr, int width, int height) = 0;
    };

    // Takes over `window`'s reference. The viewer's hands are set up as `hands` says.
    Player(ANativeWindow* window, DecoderChoice decoders, HandsOptions hands, Listener& listener);
    ~Player() override;
    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    // `protocol` is an id (stream_protocol.h on the PC): rtsp-tcp, rtsp-udp, udp, http or usb.
    // `host`, `port` and `path` aren't used for USB; an empty path is the protocol's usual one.
    // False for a protocol it doesn't know.
    bool Start(const std::string& protocol, const std::string& host, uint16_t port,
               const std::string& path);
    // A touch for the PC's mouse, on the way the stream came. Doesn't block.
    void SendPointer(const PointerEvent& event);
    void SendCameraStatus(const CameraReport& report);
    // The display's turn (Surface.ROTATION_*) and density, for the hands and the head.
    void SetScreen(int quarterTurns, float dotsPerInch);

private:
    // `hands`, the viewer's hand pointing at the window moving the PC's pointer, and the
    // camera's status going to the PC.
    HandsOptions PointingHere(HandsOptions hands);
    void OnFrame(AccessUnit&& frame) override;
    void OnStatus(PlayerStatus status, const std::string& detail) override;
    void OnVideoSize(int width, int height) override;
    void OnFirstFrame() override;
    void OnDecoderError(const std::string& message) override;
    void OnKeyFrameNeeded() override;
    void OnFrameQueued(int64_t presentationUs, const ViewInfo& view) override;

    Listener& m_listener;
    ANativeWindow* m_window;
    std::unique_ptr<VideoRenderer> m_renderer;  // null when the decoder draws straight to it
    std::unique_ptr<VideoDecoder> m_decoder;
    std::unique_ptr<StreamSource> m_source;
    std::shared_ptr<MessageLink> m_link;
    std::unique_ptr<PointerSender> m_pointer;
    std::unique_ptr<CameraStatusSender> m_cameraStatus;
    std::unique_ptr<HeadTracker> m_head;
    int m_quarterTurns = 0;

    std::mutex m_pictureMutex;
    bool m_vr = false;
    int m_width = 0;
    int m_height = 0;
};

}  // namespace gazeora
