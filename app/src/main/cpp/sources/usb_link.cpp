#include "sources/usb_link.h"

#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <thread>
#include <vector>

#include "net.h"
#include "protocols/frame_assembler.h"

namespace gazeora {

namespace {

// Accessory reads come in whole USB transfers; a smaller buffer can lose data.
constexpr size_t kReadSize = 16384;
// Not a real length: out of step. Well over any frame, but not so much that whatever is on the
// other end of the cable can have the app hold it all.
constexpr uint32_t kMaxMessage = 16 << 20;

// Where the next "GZF1" starts at or after `from`, or where the bytes run out.
size_t Resync(const std::vector<uint8_t>& data, size_t from, size_t filled) {
    for (size_t i = from; i + 4 <= filled; ++i) {
        if (StartsWithGzfMagic(data.data() + i)) {
            return i;
        }
    }
    return filled >= 3 && filled - 3 > from ? filled - 3 : from;
}

}  // namespace

UsbLink& UsbLink::Get() {
    static UsbLink* link = [] {
        auto* created = new UsbLink();  // never destroyed: its threads outlive any player
        std::thread([created] { created->Talk(); }).detach();
        return created;
    }();
    return *link;
}

void UsbLink::Attach(int fd) {
    {
        std::lock_guard<std::mutex> lock(m_writeMutex);
        if (m_fd >= 0) {
            close(fd);
            return;
        }
        m_fd = fd;
        m_open = true;
    }
    m_keyFrameWanted = true;
    std::thread([this, fd] { Read(fd); }).detach();
    if (m_watched) {
        Say(GzfType::Hello);
    }
}

void UsbLink::SetSink(SourceSink* sink) {
    {
        std::lock_guard<std::mutex> lock(m_sinkMutex);
        m_sink = sink;
        ++m_sinkGeneration;
    }
    m_watched = sink != nullptr;
    if (sink) {
        m_keyFrameWanted = true;
        Say(GzfType::Hello);
    } else {
        Say(GzfType::Bye);
    }
}

void UsbLink::Read(int fd) {
    uint32_t generation = m_sinkGeneration;
    FrameAssembler assembler(
        [this](AccessUnit&& frame) {
            std::lock_guard<std::mutex> lock(m_sinkMutex);
            if (m_sink) {
                m_sink->OnFrame(std::move(frame));
            }
        },
        [this] { m_keyFrameWanted = true; });
    std::vector<uint8_t> inbox(1 << 20);
    size_t filled = 0;
    while (true) {
        if (inbox.size() - filled < kReadSize) {
            inbox.resize(inbox.size() * 2);
        }
        ssize_t got = read(fd, inbox.data() + filled, kReadSize);
        if (got < 0 && errno == EINTR) {
            continue;
        }
        if (got <= 0) {
            break;  // unplugged
        }
        filled += size_t(got);
        if (generation != m_sinkGeneration) {
            generation = m_sinkGeneration;
            assembler.Reset();  // a new player: it starts on a key frame
        }

        size_t at = 0;
        while (filled - at >= kGzfHeaderSize) {
            GzfHeader header;
            if (!GzfHeader::Read(inbox.data() + at, filled - at, header)) {
                at = Resync(inbox, at + 1, filled);
                continue;
            }
            uint32_t length = header.tail;
            if (length > kMaxMessage) {
                at = Resync(inbox, at + 1, filled);
                continue;
            }
            if (filled - at < kGzfHeaderSize + length) {
                break;  // the rest is on its way
            }
            if (header.type == uint8_t(GzfType::Frame)) {
                header.tail = 1;  // a whole frame is one chunk; the assembler still checks it follows on
                assembler.Add(header, inbox.data() + at + kGzfHeaderSize, length, net::NowMs());
            }
            at += kGzfHeaderSize + length;
        }
        std::copy(inbox.begin() + long(at), inbox.begin() + long(filled), inbox.begin());
        filled -= at;
    }

    {
        std::lock_guard<std::mutex> lock(m_writeMutex);
        close(fd);
        m_fd = -1;
        m_open = false;
    }
    std::lock_guard<std::mutex> lock(m_sinkMutex);
    if (m_sink) {
        m_sink->OnStatus(PlayerStatus::UsbClosed, {});
    }
}

void UsbLink::Talk() {
    uint64_t nextHelloMs = 0;
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if (!m_open || !m_watched) {
            nextHelloMs = 0;
            continue;
        }
        uint64_t now = net::NowMs();
        if (now >= nextHelloMs) {
            Say(GzfType::Hello);
            nextHelloMs = now + kGzfHelloEveryMs;
        }
        if (m_keyFrameWanted.exchange(false)) {
            Say(GzfType::KeyFrameRequest);
        }
    }
}

void UsbLink::Say(GzfType type, int lossPerMille) {
    auto message = GzfMessage(type, lossPerMille);
    Write(std::vector<uint8_t>(message.begin(), message.end()));
}

void UsbLink::Write(const std::vector<uint8_t>& message) {
    std::lock_guard<std::mutex> lock(m_writeMutex);
    if (m_fd >= 0) {
        // A failure: the reader notices the cable went.
        (void)!write(m_fd, message.data(), message.size());
    }
}

void UsbSource::Start() {
    m_started = true;
    UsbLink& link = UsbLink::Get();
    link.SetSink(&m_sink);
    if (!link.Open()) {
        m_sink.OnStatus(PlayerStatus::UsbClosed, {});
    }
}

void UsbSource::Stop() {
    if (m_started) {
        m_started = false;
        UsbLink::Get().SetSink(nullptr);
    }
}

}  // namespace gazeora
