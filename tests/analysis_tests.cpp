#include "streamview/analysis/stream_analysis.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

class TestBitWriter {
public:
    void write_bits(std::uint32_t value, std::size_t count) {
        for (std::size_t i = 0; i < count; ++i) {
            const std::size_t shift = count - 1 - i;
            write_bit(((value >> shift) & 0x01U) != 0U);
        }
    }

    void write_bit(bool value) {
        if (bit_offset_ == 0) {
            data_.push_back(0);
        }
        if (value) {
            data_.back() |= static_cast<std::uint8_t>(1U << (7 - bit_offset_));
        }
        bit_offset_ = (bit_offset_ + 1) % 8;
    }

    void write_ue(std::uint32_t value) {
        const std::uint32_t code_num = value + 1;
        std::size_t bits = 0;
        for (std::uint32_t tmp = code_num; tmp > 0; tmp >>= 1U) {
            ++bits;
        }
        for (std::size_t i = 1; i < bits; ++i) {
            write_bit(false);
        }
        write_bits(code_num, bits);
    }

    std::vector<std::uint8_t> finish_rbsp() {
        write_bit(true);
        while (bit_offset_ != 0) {
            write_bit(false);
        }
        return data_;
    }

private:
    std::vector<std::uint8_t> data_;
    std::size_t bit_offset_{};
};

std::vector<std::uint8_t> make_h265_sps_payload(std::uint32_t width, std::uint32_t height) {
    TestBitWriter writer;
    writer.write_bits(0, 4);
    writer.write_bits(0, 3);
    writer.write_bit(true);
    writer.write_bits(0, 2);
    writer.write_bit(false);
    writer.write_bits(1, 5);
    writer.write_bits(0, 32);
    writer.write_bits(0, 32);
    writer.write_bits(0, 16);
    writer.write_bits(120, 8);
    writer.write_ue(0);
    writer.write_ue(1);
    writer.write_ue(width);
    writer.write_ue(height);
    writer.write_bit(false);
    writer.write_ue(0);
    writer.write_ue(0);

    auto rbsp = writer.finish_rbsp();
    std::vector<std::uint8_t> payload{0x42, 0x01};
    payload.insert(payload.end(), rbsp.begin(), rbsp.end());
    return payload;
}

void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::exit(1);
    }
}

std::vector<std::uint8_t> make_minimal_h264_stream() {
    return {
        0x00, 0x00, 0x00, 0x01,
        0x67, 0x64, 0x00, 0x1f, 0xac, 0xd9, 0x40, 0xa0,
        0x2f, 0xf9, 0x70, 0x11, 0x00, 0x00, 0x03, 0x00,
        0x01, 0x00, 0x00, 0x03, 0x00, 0x32, 0x0f, 0x18,
        0x31, 0x96,
        0x00, 0x00, 0x01,
        0x68, 0xeb, 0xec, 0xb2,
        0x00, 0x00, 0x01,
        0x65, 0x88, 0x80,
    };
}

std::vector<std::uint8_t> make_minimal_h265_stream() {
    std::vector<std::uint8_t> stream{
        0x00, 0x00, 0x00, 0x01,
        0x40, 0x01, 0x0c,
        0x00, 0x00, 0x01,
    };
    const auto sps = make_h265_sps_payload(640, 360);
    stream.insert(stream.end(), sps.begin(), sps.end());
    const std::vector<std::uint8_t> tail{
        0x00, 0x00, 0x01,
        0x44, 0x01, 0xc0,
        0x00, 0x00, 0x01,
        0x26, 0x01, 0xae,
    };
    stream.insert(stream.end(), tail.begin(), tail.end());
    return stream;
}

void test_analyzes_minimal_h264_stream() {
    const auto stream = make_minimal_h264_stream();
    const auto analysis = streamview::analysis::analyze_h264_annex_b("sample.h264", stream);

    require(analysis.input_path == "sample.h264", "analysis input path");
    require(analysis.format == "annex_b", "analysis format");
    require(analysis.codec_guess == "h264", "analysis codec");
    require(analysis.size_bytes == stream.size(), "analysis size");
    require(analysis.nals.size() == 3, "analysis NAL count");
    require(analysis.summary.sps_count == 1, "summary SPS count");
    require(analysis.summary.pps_count == 1, "summary PPS count");
    require(analysis.summary.frame_count == 1, "summary frame count");
    require(analysis.summary.keyframe_count == 1, "summary keyframe count");
    require(analysis.summary.slices.total == 1, "summary slice count");
    require(analysis.summary.slices.i == 1, "summary I slice count");
    require(analysis.summary.active_sps.has_value(), "summary active SPS");
    require(analysis.summary.active_sps->width == 640, "summary width");
    require(analysis.summary.active_sps->height == 360, "summary height");
    require(analysis.nals[0].h264.has_value() && analysis.nals[0].h264->sps.has_value(), "first NAL SPS");
    require(analysis.nals[1].h264.has_value() && analysis.nals[1].h264->pps.has_value(), "second NAL PPS");
    require(analysis.nals[2].h264.has_value() && analysis.nals[2].h264->slice.has_value(), "third NAL slice");
    require(analysis.frames.size() == 1, "H.264 frame count");
    require(analysis.frames[0].codec == "h264", "H.264 frame codec");
    require(analysis.frames[0].frame_type == "I", "H.264 frame type");
    require(analysis.frames[0].is_keyframe, "H.264 frame keyframe");
    require(analysis.frames[0].nal_indices.size() == 1 && analysis.frames[0].nal_indices[0] == 2, "H.264 frame NAL index");
    require(analysis.summary.gop_count == 1, "H.264 GOP count");
    require(analysis.gops.size() == 1, "H.264 GOP size");
    require(analysis.gops[0].start_frame_index == 0, "H.264 GOP start");
    require(analysis.gops[0].end_frame_index == 0, "H.264 GOP end");
    require(analysis.gops[0].frame_count == 1, "H.264 GOP frame count");
    require(analysis.gops[0].starts_with_keyframe, "H.264 GOP keyframe start");
}

void test_analyzes_minimal_h265_stream() {
    const auto stream = make_minimal_h265_stream();
    const auto analysis = streamview::analysis::analyze_h265_annex_b("sample.h265", stream);

    require(analysis.input_path == "sample.h265", "H.265 analysis input path");
    require(analysis.codec_guess == "h265", "H.265 analysis codec");
    require(analysis.nals.size() == 4, "H.265 NAL count");
    require(analysis.summary.vps_count == 1, "H.265 VPS count");
    require(analysis.summary.sps_count == 1, "H.265 SPS count");
    require(analysis.summary.pps_count == 1, "H.265 PPS count");
    require(analysis.summary.active_h265_sps.has_value(), "H.265 active SPS");
    require(analysis.summary.active_h265_sps->width == 640, "H.265 summary width");
    require(analysis.summary.active_h265_sps->height == 360, "H.265 summary height");
    require(analysis.summary.frame_count == 1, "H.265 frame count");
    require(analysis.summary.keyframe_count == 1, "H.265 keyframe count");
    require(analysis.summary.slices.total == 1, "H.265 VCL count");
    require(analysis.summary.slices.i == 1, "H.265 I slice count");
    require(analysis.nals[0].h265.has_value(), "first H.265 NAL");
    require(analysis.nals[0].h265->header.nal_unit_type == streamview::bitstream::H265NalType::Vps, "first H.265 VPS");
    require(analysis.nals[1].h265.has_value() && analysis.nals[1].h265->sps.has_value(), "second H.265 SPS");
    require(analysis.nals[2].h265.has_value() && analysis.nals[2].h265->pps.has_value(), "third H.265 PPS");
    require(analysis.nals[3].h265->header.nal_unit_type == streamview::bitstream::H265NalType::IdrWRadl, "fourth H.265 IDR");
    require(analysis.nals[3].h265->slice.has_value(), "fourth H.265 slice");
    require(analysis.nals[3].h265->slice->first_slice_segment_in_pic_flag, "H.265 slice first flag");
    require(analysis.nals[3].h265->slice->slice_pic_parameter_set_id == 0, "H.265 slice PPS id");
    require(analysis.nals[3].h265->slice->slice_type_present, "H.265 slice type present");
    require(analysis.nals[3].h265->slice->slice_kind == streamview::bitstream::H265SliceKind::I, "H.265 slice kind");
    require(analysis.frames.size() == 1, "H.265 frames size");
    require(analysis.frames[0].codec == "h265", "H.265 frame codec");
    require(analysis.frames[0].frame_type == "I", "H.265 frame type");
    require(analysis.frames[0].is_keyframe, "H.265 frame keyframe");
    require(analysis.frames[0].nal_indices.size() == 1 && analysis.frames[0].nal_indices[0] == 3, "H.265 frame NAL index");
    require(analysis.summary.gop_count == 1, "H.265 GOP count");
    require(analysis.gops.size() == 1, "H.265 GOP size");
    require(analysis.gops[0].start_frame_index == 0, "H.265 GOP start");
    require(analysis.gops[0].end_frame_index == 0, "H.265 GOP end");
    require(analysis.gops[0].frame_count == 1, "H.265 GOP frame count");
    require(analysis.gops[0].starts_with_keyframe, "H.265 GOP keyframe start");
}

} // namespace

int main() {
    test_analyzes_minimal_h264_stream();
    test_analyzes_minimal_h265_stream();
    return 0;
}
