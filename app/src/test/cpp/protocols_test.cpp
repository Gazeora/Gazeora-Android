#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "protocols/annex_b.h"
#include "protocols/discovery_reply.h"
#include "protocols/frame_assembler.h"
#include "protocols/gzf.h"
#include "protocols/pointer_message.h"
#include "protocols/rtp_depacketizer.h"
#include "protocols/rtsp_message.h"
#include "protocols/ts_demuxer.h"
#include "protocols/utf16.h"
#include "protocols/view_info.h"

namespace gazeora {
namespace {

using Bytes = std::vector<uint8_t>;

void Append(Bytes& to, const Bytes& more) { to.insert(to.end(), more.begin(), more.end()); }

// An H.264 key frame: SPS, PPS and an IDR slice, with 4- and 3-byte start codes.
const Bytes kSps = {0x67, 0x42, 0x00, 0x1F, 0xAA};
const Bytes kPps = {0x68, 0xCE, 0x3C, 0x80};
Bytes Idr(size_t size) {
    Bytes nal = {0x65};
    for (size_t i = 1; i < size; ++i) nal.push_back(uint8_t(0x10 + i % 200));
    return nal;
}
Bytes KeyFrame(size_t sliceSize = 40) {
    Bytes frame = {0, 0, 0, 1};
    Append(frame, kSps);
    Append(frame, {0, 0, 1});
    Append(frame, kPps);
    Append(frame, {0, 0, 0, 1});
    Append(frame, Idr(sliceSize));
    return frame;
}
Bytes DeltaFrame() { return {0, 0, 0, 1, 0x41, 0x9A, 0x22}; }

TEST(AnnexBTest, SplitsNalUnits) {
    Bytes frame = KeyFrame();
    std::vector<NalSpan> nals = SplitAnnexB(frame.data(), frame.size());
    ASSERT_EQ(nals.size(), 3u);
    EXPECT_EQ(nals[0].start, 0u);
    EXPECT_EQ(nals[0].nal, 4u);
    EXPECT_EQ(nals[0].end, 4 + kSps.size());
    EXPECT_EQ(frame[nals[1].nal], 0x68);
    EXPECT_EQ(nals[1].end - nals[1].nal, kPps.size());
    EXPECT_EQ(nals[2].end, frame.size());
    EXPECT_TRUE(ContainsKeyFrame(VideoCodec::H264, frame.data(), frame.size()));
    Bytes delta = DeltaFrame();
    EXPECT_FALSE(ContainsKeyFrame(VideoCodec::H264, delta.data(), delta.size()));
}

TEST(AnnexBTest, ParameterSetsForTheDecoder) {
    Bytes frame = KeyFrame();
    ParameterSets sets = ExtractParameterSets(VideoCodec::H264, frame.data(), frame.size());
    Bytes sps = {0, 0, 0, 1}, pps = {0, 0, 0, 1};
    Append(sps, kSps);
    Append(pps, kPps);
    EXPECT_EQ(sets.csd0, sps);
    EXPECT_EQ(sets.csd1, pps);

    // H.265: VPS, SPS and PPS together; IDR_W_RADL (19) is a key frame.
    Bytes h265 = {0, 0, 0, 1, 0x40, 0x01, 0xAA, 0, 0, 0, 1, 0x42, 0x01, 0xBB,
                  0, 0, 0, 1, 0x44, 0x01, 0xCC, 0, 0, 0, 1, 0x26, 0x01, 0xDD};
    ParameterSets hevc = ExtractParameterSets(VideoCodec::H265, h265.data(), h265.size());
    EXPECT_EQ(hevc.csd0, Bytes(h265.begin(), h265.begin() + 21));
    EXPECT_TRUE(hevc.csd1.empty());
    EXPECT_TRUE(ContainsKeyFrame(VideoCodec::H265, h265.data(), h265.size()));
}

// The view info SEI as the PC writes it (stream_view_info.cpp): NAL header, payload type 5,
// size, the id, the text, trailing bits.
Bytes ViewInfoSei(const std::string& text, bool h265 = false) {
    Bytes nal = {0, 0, 0, 1};
    Append(nal, h265 ? Bytes{0x4E, 0x01} : Bytes{0x06});
    nal.push_back(0x05);
    nal.push_back(uint8_t(16 + text.size()));
    for (char c : std::string("GazeoraViewInfo1") + text) nal.push_back(uint8_t(c));
    nal.push_back(0x80);
    return nal;
}

TEST(ViewInfoTest, ReadsWhatThePcSends) {
    Bytes frame = KeyFrame();
    Bytes sei = ViewInfoSei("vr=1 pair=1 k1=0.250 k2=0.500 scale=0.800 sep=0.520 chroma=0.010");
    frame.insert(frame.end() - 44, sei.begin(), sei.end());  // before the slice
    ViewInfo info = FindViewInfo(VideoCodec::H264, frame.data(), frame.size());
    EXPECT_TRUE(info.vr);
    EXPECT_TRUE(info.pair);
    EXPECT_FLOAT_EQ(info.k1, 0.25f);
    EXPECT_FLOAT_EQ(info.k2, 0.5f);
    EXPECT_FLOAT_EQ(info.scale, 0.8f);
    EXPECT_FLOAT_EQ(info.separation, 0.52f);
    EXPECT_FLOAT_EQ(info.chroma, 0.01f);

    Bytes h265 = ViewInfoSei("vr=1 pair=0", true);
    info = FindViewInfo(VideoCodec::H265, h265.data(), h265.size());
    EXPECT_TRUE(info.vr);
    EXPECT_FALSE(info.pair);
}

TEST(ViewInfoTest, AFrameWithoutItIsShownAsItIs) {
    Bytes frame = KeyFrame();
    EXPECT_EQ(FindViewInfo(VideoCodec::H264, frame.data(), frame.size()), ViewInfo());
    // Other SEI, and other user data, aren't it.
    Bytes other = {0, 0, 0, 1, 0x06, 0x05, 20};
    for (char c : std::string("SomeoneElsesId!!abcd")) other.push_back(uint8_t(c));
    other.push_back(0x80);
    EXPECT_FALSE(FindViewInfo(VideoCodec::H264, other.data(), other.size()).vr);
}

TEST(ViewInfoTest, SkipsEarlierMessagesAndEmulationPrevention) {
    // A recovery point message (type 6) whose payload needed an emulation prevention byte,
    // then the view info in the same NAL unit.
    Bytes frame = {0, 0, 0, 1, 0x06, 0x06, 0x03, 0x00, 0x00, 0x03, 0x01, 0x05, 16 + 4};
    for (char c : std::string("GazeoraViewInfo1vr=1")) frame.push_back(uint8_t(c));
    frame.push_back(0x80);
    EXPECT_TRUE(FindViewInfo(VideoCodec::H264, frame.data(), frame.size()).vr);
}

TEST(ViewInfoTest, AMessageLongerThanItsNalUnitIsntRead) {
    // A size of 255 * 40 + 200: far past the bytes there are.
    Bytes frame = {0, 0, 0, 1, 0x06, 0x05};
    frame.insert(frame.end(), 40, 0xFF);
    frame.push_back(200);
    for (char c : std::string("GazeoraViewInfo1vr=1")) frame.push_back(uint8_t(c));
    frame.push_back(0x80);
    EXPECT_FALSE(FindViewInfo(VideoCodec::H264, frame.data(), frame.size()).vr);
}

TEST(GzfTest, HeaderRoundTrips) {
    GzfHeader header;
    header.type = uint8_t(GzfType::Frame);
    header.flags = kGzfKeyFrame | kGzfH265;
    header.width = 1920;
    header.height = 1080;
    header.frameNumber = 0x01020304;
    header.timestamp = 0xFFFFFFF0;
    header.tail = 2 << 16 | 3;
    uint8_t raw[kGzfHeaderSize];
    header.Write(raw);
    EXPECT_EQ(std::string(reinterpret_cast<char*>(raw), 4), "GZF1");
    GzfHeader read;
    ASSERT_TRUE(GzfHeader::Read(raw, sizeof(raw), read));
    EXPECT_TRUE(read.KeyFrame());
    EXPECT_TRUE(read.H265());
    EXPECT_EQ(read.width, 1920);
    EXPECT_EQ(read.frameNumber, 0x01020304u);
    EXPECT_EQ(read.timestamp, 0xFFFFFFF0u);
    EXPECT_EQ(read.tail, uint32_t(2 << 16 | 3));
    EXPECT_FALSE(GzfHeader::Read(raw, sizeof(raw) - 1, read));

    auto hello = GzfMessage(GzfType::Hello, 2000);
    ASSERT_TRUE(GzfHeader::Read(hello.data(), hello.size(), read));
    EXPECT_EQ(read.type, uint8_t(GzfType::Hello));
    EXPECT_EQ(read.lossPerMille, 1000);  // clamped
    EXPECT_EQ(read.timestamp, 0u);       // no cookie before a challenge

    // Over UDP, the cookie of the PC's challenge goes back in the timestamp.
    auto answer = GzfMessage(GzfType::Hello, 0, 0xC00C1E5u);
    ASSERT_TRUE(GzfHeader::Read(answer.data(), answer.size(), read));
    EXPECT_EQ(read.timestamp, 0xC00C1E5u);
}

TEST(PointerMessageTest, IsAGzfMessageWithItsPayload) {
    PointerEvent event;
    event.action = PointerAction::Scroll;
    event.x = 1.0f;
    event.y = 0.5f;
    event.wheelVertical = -240;
    event.wheelHorizontal = 120;
    Bytes message = PointerMessage(event);
    ASSERT_EQ(message.size(), kGzfHeaderSize + kPointerPayloadSize);

    GzfHeader header;
    ASSERT_TRUE(GzfHeader::Read(message.data(), message.size(), header));
    EXPECT_EQ(header.type, uint8_t(GzfType::Pointer));
    EXPECT_EQ(header.tail, kPointerPayloadSize);
    const uint8_t* payload = message.data() + kGzfHeaderSize;
    EXPECT_EQ(payload[0], uint8_t(PointerAction::Scroll));
    EXPECT_EQ(payload[1], uint8_t(PointerButton::Left));
    EXPECT_EQ(payload[2] << 8 | payload[3], 65535);
    EXPECT_EQ(payload[4] << 8 | payload[5], 32768);
    EXPECT_EQ(int16_t(payload[6] << 8 | payload[7]), -240);
    EXPECT_EQ(int16_t(payload[8] << 8 | payload[9]), 120);
}

TEST(PointerMessageTest, KeepsCoordinatesInThePicture) {
    PointerEvent event;
    event.x = -0.5f;
    event.y = 2.0f;
    Bytes message = PointerMessage(event);
    const uint8_t* payload = message.data() + kGzfHeaderSize;
    EXPECT_EQ(payload[2] << 8 | payload[3], 0);
    EXPECT_EQ(payload[4] << 8 | payload[5], 65535);
}

class FrameAssemblerTest : public ::testing::Test {
protected:
    FrameAssembler assembler{[this](AccessUnit&& frame) { frames.push_back(std::move(frame)); },
                             [this] { ++keyFrameRequests; }};
    std::vector<AccessUnit> frames;
    int keyFrameRequests = 0;

    void Chunk(uint32_t frameNumber, bool key, uint32_t index, uint32_t count, const Bytes& data) {
        GzfHeader header;
        header.type = uint8_t(GzfType::Frame);
        header.flags = key ? kGzfKeyFrame : 0;
        header.width = 64;
        header.height = 48;
        header.frameNumber = frameNumber;
        header.timestamp = frameNumber * 3000;
        header.tail = index << 16 | count;
        assembler.Add(header, data.data(), data.size());
    }
};

TEST_F(FrameAssemblerTest, PutsChunksTogetherInOrder) {
    Chunk(1, true, 1, 3, {4, 5});
    Chunk(1, true, 0, 3, {1, 2, 3});
    EXPECT_TRUE(frames.empty());
    Chunk(1, true, 2, 3, {6});
    ASSERT_EQ(frames.size(), 1u);
    EXPECT_EQ(frames[0].data, Bytes({1, 2, 3, 4, 5, 6}));
    EXPECT_TRUE(frames[0].keyFrame);
    EXPECT_EQ(frames[0].width, 64);
    EXPECT_EQ(frames[0].timestamp, 3000u);
}

TEST_F(FrameAssemblerTest, SkipsToTheNextKeyFrameAfterALoss) {
    Chunk(1, true, 0, 1, {1});
    Chunk(2, false, 0, 2, {2});  // its second chunk is lost
    Chunk(3, false, 0, 1, {3});
    Chunk(4, false, 0, 1, {4});
    Chunk(5, true, 0, 1, {5});
    ASSERT_EQ(frames.size(), 2u);
    EXPECT_EQ(frames[1].data, Bytes({5}));
    EXPECT_GT(keyFrameRequests, 0);
    EXPECT_EQ(assembler.TakeLossPerMille(), 200);  // 1 of 5 lost, counted once
    EXPECT_EQ(assembler.TakeLossPerMille(), 0);
}

TEST_F(FrameAssemblerTest, WaitsForChunksThatArriveAfterTheNextFrame) {
    GzfHeader header;
    header.type = uint8_t(GzfType::Frame);
    auto chunk = [&](uint32_t frameNumber, bool key, uint32_t index, uint32_t count, uint8_t byte,
                     uint64_t nowMs) {
        header.flags = key ? kGzfKeyFrame : 0;
        header.frameNumber = frameNumber;
        header.tail = index << 16 | count;
        assembler.Add(header, &byte, 1, nowMs);
    };
    chunk(1, true, 0, 2, 1, 0);
    chunk(2, false, 0, 1, 3, 1);  // whole, but frame 1 isn't yet
    EXPECT_TRUE(frames.empty());
    EXPECT_TRUE(assembler.Waiting());
    assembler.Tick(5);
    chunk(1, true, 1, 2, 2, 6);  // late, not lost
    ASSERT_EQ(frames.size(), 2u);
    EXPECT_EQ(frames[0].data, Bytes({1, 2}));
    EXPECT_EQ(frames[1].data, Bytes({3}));
    EXPECT_EQ(keyFrameRequests, 0);
    EXPECT_EQ(assembler.TakeLossPerMille(), 0);

    chunk(3, false, 0, 2, 4, 40);  // its second chunk never comes
    chunk(4, false, 0, 1, 5, 41);
    assembler.Tick(45);
    EXPECT_EQ(frames.size(), 2u);
    assembler.Tick(52);  // given up on
    EXPECT_EQ(keyFrameRequests, 2);  // the loss, then frame 4 skipped
    EXPECT_FALSE(assembler.Waiting());
}

TEST_F(FrameAssemblerTest, StartsOverWhenThePcDoes) {
    for (uint32_t frame = 1; frame <= 500; ++frame) {
        Chunk(frame, frame == 1, 0, 1, {uint8_t(frame)});
    }
    ASSERT_EQ(frames.size(), 500u);
    // Gazeora restarted: its frames are numbered from 0 again, a key frame first.
    Chunk(0, true, 0, 1, {1});
    Chunk(1, false, 0, 1, {2});
    ASSERT_EQ(frames.size(), 502u);
    EXPECT_TRUE(frames[500].keyFrame);
}

TEST_F(FrameAssemblerTest, WaitsForAKeyFrameToStart) {
    Chunk(7, false, 0, 1, {7});
    EXPECT_TRUE(frames.empty());
    EXPECT_EQ(keyFrameRequests, 1);
    Chunk(8, true, 0, 1, {8});
    EXPECT_EQ(frames.size(), 1u);
}

// RTP packets of one frame: header, then payload.
Bytes Rtp(uint16_t sequence, uint32_t timestamp, bool marker, const Bytes& payload) {
    Bytes packet = {0x80, uint8_t(96 | (marker ? 0x80 : 0)), uint8_t(sequence >> 8),
                    uint8_t(sequence), uint8_t(timestamp >> 24), uint8_t(timestamp >> 16),
                    uint8_t(timestamp >> 8), uint8_t(timestamp), 0x12, 0x34, 0x56, 0x78};
    Append(packet, payload);
    return packet;
}

// An H.264 key frame as RFC 6184 packs it: SPS and PPS in a STAP-A, the IDR slice in FU-As.
std::vector<Bytes> KeyFramePackets(uint16_t firstSequence, uint32_t timestamp, const Bytes& idr) {
    Bytes stap = {24, 0, uint8_t(kSps.size())};
    Append(stap, kSps);
    Append(stap, {0, uint8_t(kPps.size())});
    Append(stap, kPps);
    std::vector<Bytes> packets = {Rtp(firstSequence, timestamp, false, stap)};
    const uint8_t indicator = uint8_t((idr[0] & 0xE0) | 28);
    const uint8_t type = idr[0] & 0x1F;
    size_t at = 1, piece = 16;
    while (at < idr.size()) {
        size_t size = std::min(piece, idr.size() - at);
        bool first = at == 1, last = at + size == idr.size();
        Bytes fu = {indicator, uint8_t(type | (first ? 0x80 : 0) | (last ? 0x40 : 0))};
        fu.insert(fu.end(), idr.begin() + long(at), idr.begin() + long(at + size));
        packets.push_back(Rtp(uint16_t(firstSequence + packets.size()), timestamp, last, fu));
        at += size;
    }
    return packets;
}

TEST(RtpDepacketizerTest, RebuildsAFrameFromStapAAndFuA) {
    std::vector<AccessUnit> frames;
    RtpDepacketizer depacketizer(
        VideoCodec::H264, [&](AccessUnit&& frame) { frames.push_back(std::move(frame)); },
        [] { FAIL() << "nothing was lost"; });
    for (const Bytes& packet : KeyFramePackets(65533, 9000, Idr(50))) {  // 5 packets
        depacketizer.Add(packet.data(), packet.size());
    }
    ASSERT_EQ(frames.size(), 1u);
    Bytes expected = {0, 0, 0, 1};
    Append(expected, kSps);
    Append(expected, {0, 0, 0, 1});
    Append(expected, kPps);
    Append(expected, {0, 0, 0, 1});
    Append(expected, Idr(50));
    EXPECT_EQ(frames[0].data, expected);
    EXPECT_TRUE(frames[0].keyFrame);
    EXPECT_EQ(frames[0].timestamp, 9000u);

    RtpDepacketizer::Reception reception = depacketizer.TakeReception();
    EXPECT_EQ(reception.ssrc, 0x12345678u);
    EXPECT_EQ(reception.cumulativeLost, 0u);
    EXPECT_EQ(reception.highestSequence, (1u << 16) | 1);  // wrapped once
}

TEST(RtpDepacketizerTest, PutsPacketsThatArriveOutOfOrderBack) {
    std::vector<AccessUnit> frames;
    RtpDepacketizer depacketizer(
        VideoCodec::H264, [&](AccessUnit&& frame) { frames.push_back(std::move(frame)); },
        [] { FAIL() << "nothing was lost"; });
    std::vector<Bytes> packets = KeyFramePackets(500, 0, Idr(50));
    Bytes delta = Rtp(uint16_t(500 + packets.size()), 3000, true, {0x41, 0x9A});
    // As Wi-Fi delivered them: the key frame's third packet after the next frame.
    for (size_t i : {0, 1, 3, 4}) depacketizer.Add(packets[i].data(), packets[i].size(), 1);
    depacketizer.Add(delta.data(), delta.size(), 2);
    EXPECT_TRUE(frames.empty());
    depacketizer.Tick(5);  // not given up on yet
    depacketizer.Add(packets[2].data(), packets[2].size(), 6);
    ASSERT_EQ(frames.size(), 2u);
    EXPECT_TRUE(frames[0].keyFrame);
    EXPECT_FALSE(frames[1].keyFrame);
    EXPECT_EQ(depacketizer.TakeReception().cumulativeLost, 0u);
}

TEST(RtpDepacketizerTest, SkipsToTheNextKeyFrameAfterALoss) {
    std::vector<AccessUnit> frames;
    int keyFrameRequests = 0;
    RtpDepacketizer depacketizer(
        VideoCodec::H264, [&](AccessUnit&& frame) { frames.push_back(std::move(frame)); },
        [&] { ++keyFrameRequests; });
    uint16_t sequence = 100;
    auto send = [&](const Bytes& packet) { depacketizer.Add(packet.data(), packet.size()); };

    send(Rtp(sequence++, 0, true, {0x65, 0x88}));  // a key frame
    ++sequence;                                     // a delta frame's packet is lost
    send(Rtp(sequence++, 6000, true, {0x41, 0x9A}));  // which this one refers to
    EXPECT_TRUE(depacketizer.Waiting());              // for the lost one, in case it's late
    EXPECT_EQ(keyFrameRequests, 0);
    depacketizer.Tick(100);
    EXPECT_FALSE(depacketizer.Waiting());
    EXPECT_EQ(frames.size(), 1u);
    EXPECT_GT(keyFrameRequests, 0);

    for (const Bytes& packet : KeyFramePackets(sequence, 9000, Idr(50))) {
        send(packet);
        ++sequence;
    }
    ASSERT_EQ(frames.size(), 2u);  // decoding starts over on it
    EXPECT_TRUE(frames[1].keyFrame);
    send(Rtp(sequence++, 12000, true, {0x41, 0x9B}));
    EXPECT_EQ(frames.size(), 3u);

    RtpDepacketizer::Reception reception = depacketizer.TakeReception();
    EXPECT_EQ(reception.cumulativeLost, 1u);
    EXPECT_GT(reception.fractionLost, 0);
    std::vector<uint8_t> report = BuildReceiverReport(1, reception);
    EXPECT_EQ(report.size(), 32u);
    EXPECT_EQ(report[1], 201);
    std::vector<uint8_t> pli = BuildPictureLossIndication(1, 0x12345678);
    EXPECT_EQ(pli, Bytes({0x81, 206, 0, 2, 0, 0, 0, 1, 0x12, 0x34, 0x56, 0x78}));
}

// A transport stream like Gazeora's ts_muxer: PAT and PMT, then each frame a PES whose
// first packet has the PCR (and the random access flag) and whose last has stuffing.
class TsStream {
public:
    Bytes Tables(uint8_t streamType) {
        Bytes out;
        Section(0, {0x00, 0xB0, 13, 0, 1, 0xC1, 0, 0, 0, 1, 0xF0, 0x00, 0, 0, 0, 0}, out);
        Section(0x1000, {0x02, 0xB0, 18, 0, 1, 0xC1, 0, 0, 0xE1, 0x00, 0xF0, 0x00, streamType,
                         0xE1, 0x00, 0xF0, 0x00, 0, 0, 0, 0}, out);
        return out;
    }

    Bytes Pes(const Bytes& annexB, uint32_t pts, bool key) {
        Bytes pes = {0, 0, 1, 0xE0, 0, 0, 0x80, 0x80, 5,
                     uint8_t(0x21 | ((pts >> 29) & 0x0E)), uint8_t(pts >> 22),
                     uint8_t((pts >> 14) | 1), uint8_t(pts >> 7), uint8_t((pts << 1) | 1)};
        Append(pes, annexB);
        Bytes out;
        size_t at = 0;
        bool first = true;
        while (at < pes.size()) {
            Bytes field;
            if (first) {
                field = {uint8_t(0x10 | (key ? 0x40 : 0)), 0, 0, 0, 0, 0x7E, 0};
            }
            size_t room = 184 - (first ? 1 + field.size() : 0);
            size_t left = pes.size() - at;
            bool hasField = first;
            if (left < room) {
                size_t stuffing = room - left;
                if (!hasField) {
                    hasField = true;
                    --stuffing;
                    if (stuffing > 0) {
                        field.push_back(0);
                        --stuffing;
                    }
                }
                field.insert(field.end(), stuffing, 0xFF);
                room = left;
            }
            Bytes packet = {0x47, uint8_t((first ? 0x40 : 0) | 0x01), 0x00,
                            uint8_t((hasField ? 0x30 : 0x10) | (m_continuity++ & 0x0F))};
            if (hasField) {
                packet.push_back(uint8_t(field.size()));
                Append(packet, field);
            }
            packet.insert(packet.end(), pes.begin() + long(at), pes.begin() + long(at + room));
            out.insert(out.end(), packet.begin(), packet.end());
            at += room;
            first = false;
        }
        return out;
    }

private:
    static void Section(uint16_t pid, const Bytes& section, Bytes& out) {
        Bytes packet(188, 0xFF);
        packet[0] = 0x47;
        packet[1] = uint8_t(0x40 | (pid >> 8));
        packet[2] = uint8_t(pid);
        packet[3] = 0x10;
        packet[4] = 0;
        std::copy(section.begin(), section.end(), packet.begin() + 5);
        Append(out, packet);
    }

    int m_continuity = 0;
};

TEST(TsDemuxerTest, HandsOverEachFrameAsSoonAsItsLastPacketArrives) {
    std::vector<AccessUnit> frames;
    TsDemuxer demuxer([&](AccessUnit&& frame) { frames.push_back(std::move(frame)); });
    TsStream stream;
    Bytes key = KeyFrame(500);  // several packets
    Bytes bytes = stream.Tables(0x1B);
    Append(bytes, stream.Pes(key, 90000, true));
    // In awkward pieces, as TCP may deliver them.
    for (size_t at = 0; at < bytes.size(); at += 100) {
        demuxer.Add(bytes.data() + at, std::min<size_t>(100, bytes.size() - at));
    }
    ASSERT_EQ(frames.size(), 1u);  // without waiting for the next frame
    EXPECT_EQ(frames[0].data, key);
    EXPECT_TRUE(frames[0].keyFrame);
    EXPECT_EQ(frames[0].codec, VideoCodec::H264);
    EXPECT_EQ(frames[0].timestamp, 90000u);

    Bytes delta = DeltaFrame();
    Bytes next = stream.Pes(delta, 93000, false);  // one packet: PCR and stuffing together
    demuxer.Add(next.data(), next.size());
    ASSERT_EQ(frames.size(), 2u);
    EXPECT_EQ(frames[1].data, delta);
    EXPECT_FALSE(frames[1].keyFrame);
}

TEST(TsDemuxerTest, TakesH265AndDropsAFrameWithAPacketLost) {
    std::vector<AccessUnit> frames;
    TsDemuxer demuxer([&](AccessUnit&& frame) { frames.push_back(std::move(frame)); });
    TsStream stream;
    Bytes tables = stream.Tables(0x24);
    demuxer.Add(tables.data(), tables.size());
    Bytes frame(600, 0x22);
    frame[0] = frame[1] = frame[2] = 0;
    frame[3] = 1;
    Bytes pes = stream.Pes(frame, 0, false);
    pes.erase(pes.begin() + 188, pes.begin() + 2 * 188);  // the second packet goes missing
    demuxer.Add(pes.data(), pes.size());
    EXPECT_TRUE(frames.empty());
    Bytes again = stream.Pes(frame, 3000, false);
    demuxer.Add(again.data(), again.size());
    ASSERT_EQ(frames.size(), 1u);
    EXPECT_EQ(frames[0].codec, VideoCodec::H265);
}

TEST(DiscoveryReplyTest, ReadsGazeorasReply) {
    DiscoveryReply reply;
    ASSERT_TRUE(ParseDiscoveryReply(
        R"({"service":"gazeora","version":1,"name":"Desk \"A\" é","protocol":"udp",)"
        R"("port":8554,"path":"/","codec":"h265"})",
        "10.0.0.2", reply));
    EXPECT_EQ(reply.name, "Desk \"A\" \xC3\xA9");
    EXPECT_EQ(reply.protocol, "udp");
    EXPECT_EQ(reply.port, 8554);
    EXPECT_EQ(reply.codec, "h265");

    ASSERT_TRUE(ParseDiscoveryReply(R"({"service":"gazeora","protocol":"usb"})", "10.0.0.2",
                                    reply));
    EXPECT_EQ(reply.name, "10.0.0.2");
    EXPECT_FALSE(ParseDiscoveryReply(R"({"service":"other","protocol":"udp"})", "", reply));
    EXPECT_FALSE(ParseDiscoveryReply(R"({"service":"gazeora")", "", reply));
    EXPECT_FALSE(ParseDiscoveryReply("GAZEORA-DISCOVER 1", "", reply));
}

TEST(DiscoveryReplyTest, RefusesAPathOrPortThatIsntOne) {
    DiscoveryReply reply;
    auto with = [&](const std::string& members) {
        return ParseDiscoveryReply(R"({"service":"gazeora","protocol":"http",)" + members + "}",
                                   "10.0.0.2", reply);
    };
    EXPECT_TRUE(with(R"("port":8554,"path":"/stream.ts")"));
    // A line break or space would add to the request it goes into.
    EXPECT_FALSE(with(R"("port":8554,"path":"/x HTTP/1.1\r\nX-Evil: 1")"));
    EXPECT_FALSE(with(R"("port":8554,"path":"/a b")"));
    EXPECT_FALSE(with(R"("port":8554,"path":"stream.ts")"));
    EXPECT_FALSE(with(R"("port":70000,"path":"/")"));
    EXPECT_FALSE(with(R"("port":-1,"path":"/")"));
}

TEST(Utf16Test, KeepsUtf8AndReplacesWhatIsntIt) {
    EXPECT_EQ(Utf16FromUtf8("Desk \xC3\xA9"), u"Desk \u00E9");
    EXPECT_EQ(Utf16FromUtf8("\xF0\x9F\x8E\xAE"), u"\U0001F3AE");  // a surrogate pair
    EXPECT_EQ(Utf16FromUtf8(std::string("a\0b", 3)), std::u16string(u"a\0b", 3));
    // A stray continuation byte, a sequence cut short, an overlong NUL, an encoded surrogate,
    // a byte that never starts one.
    EXPECT_EQ(Utf16FromUtf8("\x80"), u"\uFFFD");
    EXPECT_EQ(Utf16FromUtf8("x\xE2\x82"), u"x\uFFFD\uFFFD");
    EXPECT_EQ(Utf16FromUtf8("\xC0\x80"), u"\uFFFD");
    EXPECT_EQ(Utf16FromUtf8("\xED\xA0\x80"), u"\uFFFD");
    EXPECT_EQ(Utf16FromUtf8("\xFF" "ok"), u"\uFFFDok");
}

TEST(RtspMessageTest, ParsesResponsesWithBodies) {
    std::string text =
        "RTSP/1.0 200 OK\r\nCSeq: 2\r\ncontent-base: rtsp://10.0.2.2:8554/\r\n"
        "Content-Length: 4\r\n\r\nv=0\nRTSP/1.0 2";
    Response response;
    long taken = ParseResponse(text.data(), text.size(), true, response);
    EXPECT_EQ(taken, long(text.size() - 10));
    EXPECT_EQ(response.status, 200);
    EXPECT_EQ(response.reason, "OK");
    EXPECT_EQ(response.Header("Content-Base"), "rtsp://10.0.2.2:8554/");
    EXPECT_EQ(response.body, "v=0\n");
    EXPECT_EQ(ParseResponse(text.data() + taken, 10, true, response), 0);  // not all there
    EXPECT_EQ(ParseResponse("$\x00\x00\x04", 4, true, response), -1);

    std::string http = "HTTP/1.1 404 Not Found\r\nServer: Gazeora\r\n\r\n\x47";
    EXPECT_EQ(ParseResponse(http.data(), http.size(), false, response), long(http.size() - 1));
    EXPECT_EQ(response.status, 404);

    EXPECT_EQ(SessionId("ABCDEF;timeout=60"), "ABCDEF");
    EXPECT_EQ(SessionTimeoutSeconds("ABCDEF;timeout=30"), 30);
    EXPECT_EQ(SessionTimeoutSeconds("ABCDEF"), 60);
    EXPECT_EQ(TransportParameter("RTP/AVP;unicast;client_port=5000-5001;server_port=6970-6971",
                                 "server_port"),
              "6970-6971");
}

TEST(RtspMessageTest, FindsTheVideoInTheSdp) {
    std::string sdp =
        "v=0\r\no=- 0 0 IN IP4 0.0.0.0\r\ns=Gazeora\r\nt=0 0\r\na=control:*\r\n"
        "m=audio 0 RTP/AVP 97\r\na=rtpmap:97 opus/48000/2\r\na=control:track0\r\n"
        "m=video 0 RTP/AVP 96\r\na=rtpmap:96 H265/90000\r\n"
        "a=fmtp:96 sprop-vps=QAE=\r\na=control:track1\r\n";
    SdpVideo video;
    ASSERT_TRUE(ParseSdpVideo(sdp, video));
    EXPECT_EQ(video.codec, VideoCodec::H265);
    EXPECT_EQ(video.payloadType, 96);
    EXPECT_EQ(video.control, "track1");
    EXPECT_FALSE(ParseSdpVideo("v=0\r\nm=audio 0 RTP/AVP 97\r\n", video));

    EXPECT_EQ(ResolveControl("rtsp://pc:8554/", "track1"), "rtsp://pc:8554/track1");
    EXPECT_EQ(ResolveControl("rtsp://pc:8554", "track1"), "rtsp://pc:8554/track1");
    EXPECT_EQ(ResolveControl("rtsp://pc:8554/", "*"), "rtsp://pc:8554/");
    EXPECT_EQ(ResolveControl("rtsp://pc:8554/", "rtsp://other/x"), "rtsp://other/x");
}

}  // namespace
}  // namespace gazeora
