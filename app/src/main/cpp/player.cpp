#include "player.h"

#include <android/log.h>

#include "sources/http_source.h"
#include "sources/rtsp_source.h"
#include "sources/udp_source.h"
#include "sources/usb_link.h"

namespace gazeora {

Player::Player(ANativeWindow* window, DecoderChoice decoders, HandsOptions hands,
               Listener& listener)
    : m_listener(listener),
      m_window(window),
      m_renderer(std::make_unique<VideoRenderer>(window, PointingHere(std::move(hands)))) {
    std::string error;
    if (!m_renderer->Start(error)) {
        __android_log_print(ANDROID_LOG_WARN, "GazeoraPlayer", "showing frames as they are: %s",
                            error.c_str());
        m_renderer.reset();
    }
    ANativeWindow* target = m_renderer ? m_renderer->DecoderWindow() : m_window;
    m_decoder = std::make_unique<VideoDecoder>(target, std::move(decoders),
                                               static_cast<VideoDecoder::Listener&>(*this));
    if (m_renderer) {
        m_listener.OnVideoSize(0, 0);  // the renderer fits the picture to the screen
    }
}

Player::~Player() {
    // The source first, so no frames arrive at a decoder that's gone; the decoder can still
    // ask the source for a key frame until it's gone too. The renderer outlives the decoder
    // drawing into it, and the window outlives both.
    m_pointer.reset();
    if (m_source) {
        m_source->Stop();
    }
    m_decoder.reset();
    m_source.reset();
    m_renderer.reset();
    ANativeWindow_release(m_window);
}

bool Player::Start(const std::string& protocol, const std::string& host, uint16_t port,
                   const std::string& path) {
    SourceSink& sink = *this;
    if (protocol == "rtsp-tcp" || protocol == "rtsp-udp") {
        m_source = std::make_unique<RtspSource>(host, port, path, protocol == "rtsp-tcp", sink);
    } else if (protocol == "udp") {
        m_source = std::make_unique<UdpSource>(host, port, sink);
    } else if (protocol == "http") {
        m_source = std::make_unique<HttpSource>(host, port, path, sink);
    } else if (protocol == "usb") {
        m_source = std::make_unique<UsbSource>(sink);
    } else {
        return false;
    }
    m_pointer = CreatePointerSender(protocol, host);
    m_source->Start();
    return true;
}

HandsOptions Player::PointingHere(HandsOptions hands) {
    hands.pointer = [this](const PointerEvent& event) { SendPointer(event); };
    return hands;
}

void Player::SendPointer(const PointerEvent& event) {
    if (m_pointer) {
        m_pointer->Send(event);
    }
}

void Player::SetScreen(int quarterTurns, float dotsPerInch) {
    if (m_renderer) {
        m_renderer->SetScreen(quarterTurns, dotsPerInch);
    }
}

void Player::OnFrame(AccessUnit&& frame) { m_decoder->Submit(std::move(frame)); }

void Player::OnStatus(PlayerStatus status, const std::string& detail) {
    m_listener.OnStatus(status, detail);
}

void Player::OnVideoSize(int width, int height) {
    if (!m_renderer) {
        m_listener.OnVideoSize(width, height);
    }
    std::unique_lock<std::mutex> lock(m_pictureMutex);
    if (width == m_width && height == m_height) {
        return;
    }
    m_width = width;
    m_height = height;
    bool vr = m_vr;
    lock.unlock();
    m_listener.OnPicture(vr, width, height);
}

void Player::OnFrameQueued(int64_t presentationUs, const ViewInfo& view) {
    if (m_renderer) {
        m_renderer->Expect(presentationUs, view);
    }
    std::unique_lock<std::mutex> lock(m_pictureMutex);
    if (view.vr == m_vr) {
        return;
    }
    m_vr = view.vr;
    int width = m_width, height = m_height;
    lock.unlock();
    m_listener.OnPicture(view.vr, width, height);
}

void Player::OnFirstFrame() { m_listener.OnFirstFrame(); }

void Player::OnDecoderError(const std::string& message) {
    m_listener.OnStatus(PlayerStatus::Failed, message);
}

void Player::OnKeyFrameNeeded() {
    if (m_source) {
        m_source->RequestKeyFrame();
    }
}

}  // namespace gazeora
