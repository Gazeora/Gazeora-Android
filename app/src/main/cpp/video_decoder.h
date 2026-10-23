#pragma once

#include <android/native_window.h>
#include <media/NdkMediaCodec.h>

#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

#include "protocols/access_unit.h"
#include "protocols/annex_b.h"
#include "protocols/view_info.h"

namespace gazeora {

// Which decoder to use for each codec, as the app picked it (Decoders.java: the NDK can't
// list them), and whether it can decode with low latency. An empty name: the default one.
struct DecoderChoice {
    std::string avc;
    std::string hevc;
    bool avcLowLatency = false;
    bool hevcLowLatency = false;
};

// Decodes frames to a window with MediaCodec, and shows each as soon as it's decoded: no
// buffering, as for a game. The decoder is set up from a key frame's parameter sets, and
// again when the codec or the parameter sets change. A decoder that falls behind drops what's
// waiting and starts over on a key frame. All MediaCodec calls are on its own thread.
class VideoDecoder {
public:
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void OnVideoSize(int width, int height) = 0;
        virtual void OnFirstFrame() = 0;
        virtual void OnDecoderError(const std::string& message) = 0;
        // The next frame it can use is a key frame.
        virtual void OnKeyFrameNeeded() = 0;
        // A frame goes to the decoder: its picture will carry this presentation time.
        virtual void OnFrameQueued(int64_t presentationUs, const ViewInfo& view) = 0;
    };

    // `window` must outlive the decoder.
    VideoDecoder(ANativeWindow* window, DecoderChoice choice, Listener& listener);
    ~VideoDecoder();
    VideoDecoder(const VideoDecoder&) = delete;
    VideoDecoder& operator=(const VideoDecoder&) = delete;

    // From any thread.
    void Submit(AccessUnit&& frame);

private:
    void Run();
    void Decode(AccessUnit& frame);
    bool Configure(const AccessUnit& keyFrame, ParameterSets sets);
    void Drain(int64_t timeoutUs);
    void ReportSize();
    void NeedKeyFrame();
    void ReleaseCodec();
    int64_t PresentationUs(uint32_t timestamp);

    ANativeWindow* m_window;
    DecoderChoice m_choice;
    Listener& m_listener;

    std::mutex m_mutex;
    std::condition_variable m_wake;
    std::deque<AccessUnit> m_queue;
    bool m_stopping = false;
    std::thread m_thread;

    // The decoder thread's.
    AMediaCodec* m_codec = nullptr;
    VideoCodec m_codecType = VideoCodec::H264;
    ParameterSets m_sets;
    int m_width = 0;
    int m_height = 0;
    int m_shownWidth = 0;
    int m_shownHeight = 0;
    bool m_waitingForKeyFrame = true;
    bool m_shownFirst = false;
    bool m_failed = false;
    int64_t m_lastTimestamp = -1;
    int64_t m_timestampWraps = 0;
};

}  // namespace gazeora
