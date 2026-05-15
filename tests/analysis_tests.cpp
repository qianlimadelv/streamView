#include "streamview/analysis/stream_analysis.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

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
    return {
        0x00, 0x00, 0x00, 0x01,
        0x40, 0x01, 0x0c,
        0x00, 0x00, 0x01,
        0x42, 0x01, 0x01,
        0x00, 0x00, 0x01,
        0x44, 0x01, 0xc0,
        0x00, 0x00, 0x01,
        0x26, 0x01, 0x80,
    };
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
    require(analysis.summary.frame_count == 1, "H.265 frame count");
    require(analysis.summary.keyframe_count == 1, "H.265 keyframe count");
    require(analysis.summary.slices.total == 1, "H.265 VCL count");
    require(analysis.nals[0].h265.has_value(), "first H.265 NAL");
    require(analysis.nals[0].h265->header.nal_unit_type == streamview::bitstream::H265NalType::Vps, "first H.265 VPS");
    require(analysis.nals[3].h265->header.nal_unit_type == streamview::bitstream::H265NalType::IdrWRadl, "fourth H.265 IDR");
    require(analysis.frames.size() == 1, "H.265 frames size");
    require(analysis.frames[0].codec == "h265", "H.265 frame codec");
    require(analysis.frames[0].frame_type == "IDR", "H.265 frame type");
    require(analysis.frames[0].is_keyframe, "H.265 frame keyframe");
    require(analysis.frames[0].nal_indices.size() == 1 && analysis.frames[0].nal_indices[0] == 3, "H.265 frame NAL index");
}

} // namespace

int main() {
    test_analyzes_minimal_h264_stream();
    test_analyzes_minimal_h265_stream();
    return 0;
}
