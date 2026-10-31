// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "video_decoder.h"

#include <media/NdkMediaFormat.h>

#include <algorithm>
#include <chrono>
#include <cstring>

namespace gazeora {

namespace {

// Frames waiting beyond this mean the decoder can't keep up.
constexpr size_t kMaxWaiting = 4;
// How long to wait for a free input buffer before giving the frame up.
constexpr int kInputTries = 10;
constexpr int64_t kInputWaitUs = 5000;
// With nothing to feed, how long to wait for a picture before looking for frames again: a
// picture goes up the moment it's decoded, a frame waits this long at most.
constexpr int64_t kOutputWaitUs = 2000;
// Room for the largest frame: a key frame at the PC's highest quality (35 Mbit/s) can pass
// 1 MB, and one that doesn't fit is lost, and the next key frame with it.
constexpr int kMinInputSize = 4 << 20;
// For a stream that doesn't say its size; the decoder takes the real one from the stream.
constexpr int kDefaultWidth = 1920;
constexpr int kDefaultHeight = 1080;

const char* Mime(VideoCodec codec) {
    return codec == VideoCodec::H265 ? "video/hevc" : "video/avc";
}

}  // namespace

VideoDecoder::VideoDecoder(ANativeWindow* window, DecoderChoice choice, Listener& listener)
    : m_window(window), m_choice(std::move(choice)), m_listener(listener) {
    m_thread = std::thread([this] { Run(); });
}

VideoDecoder::~VideoDecoder() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stopping = true;
    }
    m_wake.notify_all();
    m_thread.join();
}

void VideoDecoder::Submit(AccessUnit&& frame) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push_back(std::move(frame));
    }
    m_wake.notify_one();
}

void VideoDecoder::Run() {
    while (true) {
        AccessUnit frame;
        bool have = false, overflow = false;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (!m_codec || m_queue.empty()) {
                // With a decoder, Drain below did the waiting.
                m_wake.wait_for(lock, std::chrono::milliseconds(m_codec ? 0 : 100),
                                [this] { return m_stopping || !m_queue.empty(); });
            }
            if (m_stopping) {
                break;
            }
            if (m_queue.size() > kMaxWaiting) {
                m_queue.clear();
                overflow = true;
            } else if (!m_queue.empty()) {
                frame = std::move(m_queue.front());
                m_queue.pop_front();
                have = true;
            }
        }
        if (overflow) {
            NeedKeyFrame();
        }
        if (have) {
            Decode(frame);
        }
        if (m_codec) {
            bool idle;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                idle = m_queue.empty();
            }
            Drain(idle ? kOutputWaitUs : 0);
        }
    }
    ReleaseCodec();
}

void VideoDecoder::Decode(AccessUnit& frame) {
    if (m_failed) {
        return;
    }
    ParameterSets sets;
    if (frame.keyFrame) {
        sets = ExtractParameterSets(frame.codec, frame.data.data(), frame.data.size());
    }
    bool sizeChanged = frame.width != 0 && (frame.width != m_width || frame.height != m_height);
    bool setsChanged = !sets.Empty() && sets != m_sets;
    if (!m_codec || frame.codec != m_codecType || sizeChanged || setsChanged) {
        if (!frame.keyFrame || sets.Empty()) {
            NeedKeyFrame();
            return;
        }
        if (!Configure(frame, std::move(sets))) {
            return;
        }
    }
    if (m_waitingForKeyFrame && !frame.keyFrame) {
        NeedKeyFrame();
        return;
    }
    m_waitingForKeyFrame = false;

    ssize_t index = -1;
    for (int i = 0; i < kInputTries && index < 0; ++i) {
        index = AMediaCodec_dequeueInputBuffer(m_codec, kInputWaitUs);
        if (index < 0) {
            Drain(0);
        }
    }
    if (index < 0) {
        NeedKeyFrame();  // the decoder is stuck behind: this frame is lost
        return;
    }
    size_t capacity = 0;
    uint8_t* input = AMediaCodec_getInputBuffer(m_codec, size_t(index), &capacity);
    if (!input || frame.data.size() > capacity) {
        AMediaCodec_queueInputBuffer(m_codec, size_t(index), 0, 0, 0, 0);
        NeedKeyFrame();
        return;
    }
    std::memcpy(input, frame.data.data(), frame.data.size());
    int64_t presentationUs = PresentationUs(frame.timestamp);
    m_listener.OnFrameQueued(presentationUs,
                             FindViewInfo(frame.codec, frame.data.data(), frame.data.size()));
    media_status_t status = AMediaCodec_queueInputBuffer(m_codec, size_t(index), 0,
                                                         frame.data.size(),
                                                         uint64_t(presentationUs), 0);
    if (status != AMEDIA_OK) {
        ReleaseCodec();
        m_failed = true;
        m_listener.OnDecoderError("The decoder stopped (error " + std::to_string(status) + ")");
    }
}

bool VideoDecoder::Configure(const AccessUnit& keyFrame, ParameterSets sets) {
    ReleaseCodec();
    const char* mime = Mime(keyFrame.codec);
    const bool h265 = keyFrame.codec == VideoCodec::H265;
    const std::string& name = h265 ? m_choice.hevc : m_choice.avc;
    const bool lowLatency = h265 ? m_choice.hevcLowLatency : m_choice.avcLowLatency;
    int width = keyFrame.width ? keyFrame.width : kDefaultWidth;
    int height = keyFrame.height ? keyFrame.height : kDefaultHeight;

    AMediaCodec* codec = name.empty() ? AMediaCodec_createDecoderByType(mime)
                                      : AMediaCodec_createCodecByName(name.c_str());
    if (!codec) {
        m_failed = true;
        m_listener.OnDecoderError(std::string("No ") + (h265 ? "H.265" : "H.264") +
                                  " decoder on this device");
        return false;
    }
    AMediaFormat* format = AMediaFormat_new();
    AMediaFormat_setString(format, AMEDIAFORMAT_KEY_MIME, mime);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_WIDTH, width);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_HEIGHT, height);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_MAX_INPUT_SIZE,
                          std::max(kMinInputSize, int(keyFrame.data.size()) * 2));
    AMediaFormat_setInt32(format, "priority", 0);  // real time
    if (lowLatency) {
        AMediaFormat_setInt32(format, "low-latency", 1);
    }
    AMediaFormat_setBuffer(format, "csd-0", sets.csd0.data(), sets.csd0.size());
    if (!sets.csd1.empty()) {
        AMediaFormat_setBuffer(format, "csd-1", sets.csd1.data(), sets.csd1.size());
    }
    media_status_t status = AMediaCodec_configure(codec, format, m_window, nullptr, 0);
    AMediaFormat_delete(format);
    if (status == AMEDIA_OK) {
        status = AMediaCodec_start(codec);
    }
    if (status != AMEDIA_OK) {
        AMediaCodec_delete(codec);
        m_failed = true;
        m_listener.OnDecoderError("Decoder " + (name.empty() ? std::string(mime) : name) +
                                  " failed (error " + std::to_string(status) + ")");
        return false;
    }
    m_codec = codec;
    m_codecType = keyFrame.codec;
    m_sets = std::move(sets);
    m_width = keyFrame.width;
    m_height = keyFrame.height;
    if (keyFrame.width) {
        m_shownWidth = keyFrame.width;
        m_shownHeight = keyFrame.height;
        m_listener.OnVideoSize(m_shownWidth, m_shownHeight);
    }
    return true;
}

void VideoDecoder::Drain(int64_t timeoutUs) {
    while (m_codec) {
        AMediaCodecBufferInfo info;
        ssize_t index = AMediaCodec_dequeueOutputBuffer(m_codec, &info, timeoutUs);
        timeoutUs = 0;  // only the first wait
        if (index >= 0) {
            bool picture = info.size > 0 || !(info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
            AMediaCodec_releaseOutputBuffer(m_codec, size_t(index), picture);  // show it now
            if (picture && !m_shownFirst) {
                m_shownFirst = true;
                m_listener.OnFirstFrame();
            }
        } else if (index == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
            ReportSize();
        } else if (index != AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED) {
            return;  // nothing more yet
        }
    }
}

void VideoDecoder::ReportSize() {
    AMediaFormat* format = AMediaCodec_getOutputFormat(m_codec);
    if (!format) {
        return;
    }
    int32_t width = 0, height = 0, left, top, right, bottom;
    if (AMediaFormat_getRect(format, AMEDIAFORMAT_KEY_DISPLAY_CROP, &left, &top, &right,
                             &bottom)) {
        width = right - left + 1;
        height = bottom - top + 1;
    } else {
        AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_WIDTH, &width);
        AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_HEIGHT, &height);
    }
    AMediaFormat_delete(format);
    if (width > 0 && height > 0 && (width != m_shownWidth || height != m_shownHeight)) {
        m_shownWidth = width;
        m_shownHeight = height;
        m_listener.OnVideoSize(width, height);
    }
}

void VideoDecoder::NeedKeyFrame() {
    m_waitingForKeyFrame = true;
    m_listener.OnKeyFrameNeeded();
}

void VideoDecoder::ReleaseCodec() {
    if (m_codec) {
        AMediaCodec_stop(m_codec);
        AMediaCodec_delete(m_codec);
        m_codec = nullptr;
    }
    m_sets = ParameterSets();
    m_waitingForKeyFrame = true;
}

int64_t VideoDecoder::PresentationUs(uint32_t timestamp) {
    // The 90 kHz clock, unwrapped past 32 bits.
    if (m_lastTimestamp >= 0 && timestamp < m_lastTimestamp &&
        m_lastTimestamp - timestamp > 0x80000000LL) {
        m_timestampWraps += 1LL << 32;
    }
    m_lastTimestamp = timestamp;
    return (m_timestampWraps + timestamp) * 1000 / 90;
}

}  // namespace gazeora
