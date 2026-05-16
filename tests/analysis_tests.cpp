#include "streamview/analysis/stream_analysis.hpp"
#include "streamview/analysis/validation.hpp"

#include <algorithm>
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
    writer.write_ue(0);

    auto rbsp = writer.finish_rbsp();
    std::vector<std::uint8_t> payload{0x42, 0x01};
    payload.insert(payload.end(), rbsp.begin(), rbsp.end());
    return payload;
}

std::vector<std::uint8_t> make_h265_vps_payload() {
    TestBitWriter writer;
    writer.write_bits(0, 4);
    writer.write_bit(true);
    writer.write_bit(true);
    writer.write_bits(0, 6);
    writer.write_bits(0, 3);
    writer.write_bit(true);
    writer.write_bits(0xffff, 16);
    writer.write_bits(0, 2);
    writer.write_bit(false);
    writer.write_bits(1, 5);
    writer.write_bits(0, 32);
    writer.write_bits(0, 32);
    writer.write_bits(0, 16);
    writer.write_bits(120, 8);
    writer.write_bit(false);
    writer.write_ue(0);
    writer.write_ue(0);
    writer.write_ue(0);
    writer.write_bits(0, 6);
    writer.write_ue(0);

    auto rbsp = writer.finish_rbsp();
    std::vector<std::uint8_t> payload{0x40, 0x01};
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

std::vector<std::uint8_t> make_h264_poc_stream() {
    TestBitWriter slice_writer;
    slice_writer.write_ue(0);
    slice_writer.write_ue(7);
    slice_writer.write_ue(0);
    slice_writer.write_bits(0, 4);
    slice_writer.write_ue(2);
    slice_writer.write_bits(5, 6);

    auto slice_rbsp = slice_writer.finish_rbsp();

    std::vector<std::uint8_t> stream{
        0x00, 0x00, 0x00, 0x01,
        0x67, 0x64, 0x00, 0x1f, 0xac, 0xd9, 0x40, 0xa0,
        0x2f, 0xf9, 0x70, 0x11, 0x00, 0x00, 0x03, 0x00,
        0x01, 0x00, 0x00, 0x03, 0x00, 0x32, 0x0f, 0x18,
        0x31, 0x96,
        0x00, 0x00, 0x01,
        0x68, 0xeb, 0xec, 0xb2,
        0x00, 0x00, 0x01,
        0x65,
    };
    stream.insert(stream.end(), slice_rbsp.begin(), slice_rbsp.end());
    return stream;
}

std::vector<std::uint8_t> make_minimal_h265_stream() {
    std::vector<std::uint8_t> stream{
        0x00, 0x00, 0x00, 0x01,
    };
    const auto vps = make_h265_vps_payload();
    stream.insert(stream.end(), vps.begin(), vps.end());
    const std::vector<std::uint8_t> sps_start{0x00, 0x00, 0x01};
    stream.insert(stream.end(), sps_start.begin(), sps_start.end());
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
    require(analysis.summary.parse_errors.total == 0, "summary parse errors");
    require(analysis.summary.active_sps.has_value(), "summary active SPS");
    require(analysis.summary.active_sps->width == 640, "summary width");
    require(analysis.summary.active_sps->height == 360, "summary height");
    require(analysis.nals[0].h264.has_value() && analysis.nals[0].h264->sps.has_value(), "first NAL SPS");
    require(analysis.nals[1].h264.has_value() && analysis.nals[1].h264->pps.has_value(), "second NAL PPS");
    require(analysis.nals[2].h264.has_value() && analysis.nals[2].h264->slice.has_value(), "third NAL slice");
    require(analysis.frames.size() == 1, "H.264 frame count");
    require(analysis.frames[0].decode_order_index == 0, "H.264 frame decode order");
    require(analysis.frames[0].gop_index.has_value(), "H.264 frame gop index present");
    require(analysis.frames[0].gop_index.value() == 0, "H.264 frame gop index");
    require(!analysis.frames[0].poc.has_value(), "H.264 frame poc absent");
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

void test_analyzes_h264_frame_poc_from_context() {
    const auto stream = make_h264_poc_stream();
    const auto analysis = streamview::analysis::analyze_h264_annex_b("poc.h264", stream);

    require(analysis.summary.frame_count == 1, "POC H.264 frame count");
    require(analysis.frames.size() == 1, "POC H.264 frames size");
    require(analysis.frames[0].gop_index.has_value(), "POC H.264 frame gop index present");
    require(analysis.frames[0].gop_index.value() == 0, "POC H.264 frame gop index");
    require(analysis.frames[0].poc.has_value(), "POC H.264 frame poc present");
    require(analysis.frames[0].poc.value() == 5, "POC H.264 frame poc");
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
    require(analysis.summary.active_h265_vps.has_value(), "H.265 active VPS");
    require(analysis.summary.active_h265_vps->profile_idc == 1, "H.265 summary VPS profile");
    require(analysis.summary.active_h265_vps->level_idc == 120, "H.265 summary VPS level");
    require(analysis.summary.active_h265_sps.has_value(), "H.265 active SPS");
    require(analysis.summary.active_h265_sps->log2_max_pic_order_cnt_lsb_minus4 == 0, "H.265 summary poc lsb shift");
    require(analysis.summary.active_h265_sps->width == 640, "H.265 summary width");
    require(analysis.summary.active_h265_sps->height == 360, "H.265 summary height");
    require(analysis.summary.frame_count == 1, "H.265 frame count");
    require(analysis.summary.keyframe_count == 1, "H.265 keyframe count");
    require(analysis.summary.slices.total == 1, "H.265 VCL count");
    require(analysis.summary.slices.i == 1, "H.265 I slice count");
    require(analysis.summary.parse_errors.total == 0, "H.265 parse errors");
    require(analysis.nals[0].h265.has_value(), "first H.265 NAL");
    require(analysis.nals[0].h265->header.nal_unit_type == streamview::bitstream::H265NalType::Vps, "first H.265 VPS");
    require(analysis.nals[0].h265->vps.has_value(), "first H.265 VPS info");
    require(analysis.nals[1].h265.has_value() && analysis.nals[1].h265->sps.has_value(), "second H.265 SPS");
    require(analysis.nals[2].h265.has_value() && analysis.nals[2].h265->pps.has_value(), "third H.265 PPS");
    require(analysis.nals[3].h265->header.nal_unit_type == streamview::bitstream::H265NalType::IdrWRadl, "fourth H.265 IDR");
    require(analysis.nals[3].h265->slice.has_value(), "fourth H.265 slice");
    require(analysis.nals[3].h265->slice->first_slice_segment_in_pic_flag, "H.265 slice first flag");
    require(analysis.nals[3].h265->slice->slice_pic_parameter_set_id == 0, "H.265 slice PPS id");
    require(analysis.nals[3].h265->slice->slice_type_present, "H.265 slice type present");
    require(analysis.nals[3].h265->slice->slice_kind == streamview::bitstream::H265SliceKind::I, "H.265 slice kind");
    require(analysis.frames.size() == 1, "H.265 frames size");
    require(analysis.frames[0].decode_order_index == 0, "H.265 frame decode order");
    require(analysis.frames[0].gop_index.has_value(), "H.265 frame gop index present");
    require(analysis.frames[0].gop_index.value() == 0, "H.265 frame gop index");
    require(!analysis.frames[0].poc.has_value(), "H.265 frame poc absent");
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

void test_analyzes_h265_frame_poc_from_context() {
    TestBitWriter writer;
    writer.write_bit(true);
    writer.write_ue(0);
    writer.write_ue(1);
    writer.write_bits(5, 4);
    auto slice_rbsp = writer.finish_rbsp();

    std::vector<std::uint8_t> stream{
        0x00, 0x00, 0x00, 0x01,
    };
    const auto vps = make_h265_vps_payload();
    stream.insert(stream.end(), vps.begin(), vps.end());
    const std::vector<std::uint8_t> sps_start{0x00, 0x00, 0x01};
    stream.insert(stream.end(), sps_start.begin(), sps_start.end());
    const auto sps = make_h265_sps_payload(640, 360);
    stream.insert(stream.end(), sps.begin(), sps.end());
    const std::vector<std::uint8_t> pps_start{0x00, 0x00, 0x01};
    stream.insert(stream.end(), pps_start.begin(), pps_start.end());
    const std::vector<std::uint8_t> pps{0x44, 0x01, 0xc0};
    stream.insert(stream.end(), pps.begin(), pps.end());
    const std::vector<std::uint8_t> slice_start{0x00, 0x00, 0x01};
    stream.insert(stream.end(), slice_start.begin(), slice_start.end());
    const std::vector<std::uint8_t> slice_header{0x02, 0x01};
    stream.insert(stream.end(), slice_header.begin(), slice_header.end());
    stream.insert(stream.end(), slice_rbsp.begin(), slice_rbsp.end());

    const auto analysis = streamview::analysis::analyze_h265_annex_b("poc.h265", stream);

    require(analysis.summary.frame_count == 1, "H.265 POC frame count");
    require(analysis.frames.size() == 1, "H.265 POC frames size");
    require(analysis.frames[0].gop_index.has_value(), "H.265 POC frame gop index present");
    require(analysis.frames[0].gop_index.value() == 0, "H.265 POC frame gop index");
    require(analysis.frames[0].poc.has_value(), "H.265 POC frame poc present");
    require(analysis.frames[0].poc.value() == 5, "H.265 POC frame poc");
}

void test_validates_duplicate_h264_parameter_sets() {
    auto stream = make_minimal_h264_stream();
    const auto duplicate = stream;
    stream.insert(stream.end(), duplicate.begin(), duplicate.end());

    const auto analysis = streamview::analysis::analyze_h264_annex_b("dup.h264", stream);
    const auto issues = streamview::analysis::validate_stream_analysis(analysis);

    require(std::any_of(issues.begin(), issues.end(), [](const auto& issue) {
                return issue.code == "duplicate_h264_sps_id" && issue.severity == "warning";
            }),
            "H.264 duplicate SPS validation issue");
    require(std::any_of(issues.begin(), issues.end(), [](const auto& issue) {
                return issue.code == "duplicate_h264_pps_id" && issue.severity == "warning";
            }),
            "H.264 duplicate PPS validation issue");
}

void test_validates_duplicate_h265_parameter_sets() {
    auto stream = make_minimal_h265_stream();
    const auto duplicate = stream;
    stream.insert(stream.end(), duplicate.begin(), duplicate.end());

    const auto analysis = streamview::analysis::analyze_h265_annex_b("dup.h265", stream);
    const auto issues = streamview::analysis::validate_stream_analysis(analysis);

    require(std::any_of(issues.begin(), issues.end(), [](const auto& issue) {
                return issue.code == "duplicate_h265_vps_id" && issue.severity == "warning";
            }),
            "H.265 duplicate VPS validation issue");
    require(std::any_of(issues.begin(), issues.end(), [](const auto& issue) {
                return issue.code == "duplicate_h265_sps_id" && issue.severity == "warning";
            }),
            "H.265 duplicate SPS validation issue");
    require(std::any_of(issues.begin(), issues.end(), [](const auto& issue) {
                return issue.code == "duplicate_h265_pps_id" && issue.severity == "warning";
            }),
            "H.265 duplicate PPS validation issue");
}

void test_validates_frame_gop_consistency() {
    streamview::analysis::StreamAnalysis analysis{};
    analysis.input_path = "synthetic.h264";
    analysis.codec_guess = "h264";
    analysis.summary.frame_count = 1;
    analysis.summary.keyframe_count = 1;
    analysis.summary.gop_count = 1;
    analysis.frames.push_back({
        .index = 0,
        .decode_order_index = 0,
        .gop_index = 0,
        .codec = "h264",
        .frame_type = "I",
        .is_keyframe = true,
        .poc = std::nullopt,
        .nal_indices = {0},
        .size_bytes = 10,
        .first_payload_offset = 0,
    });
    analysis.gops.push_back({
        .index = 0,
        .start_frame_index = 1,
        .end_frame_index = 1,
        .frame_count = 1,
        .keyframe_index = 2,
        .size_bytes = 10,
        .starts_with_keyframe = false,
    });

    const auto issues = streamview::analysis::validate_stream_analysis(analysis);

    require(std::any_of(issues.begin(), issues.end(), [](const auto& issue) {
                return issue.code == "frame_gop_index_mismatch" && issue.severity == "error";
            }),
            "frame gop mismatch validation issue");
    require(std::any_of(issues.begin(), issues.end(), [](const auto& issue) {
                return issue.code == "invalid_gop_keyframe_index" && issue.severity == "error";
            }),
            "gop keyframe validation issue");
}

void test_validates_resolution_change_detection() {
    streamview::analysis::StreamAnalysis analysis{};
    analysis.input_path = "resolution-change.h264";
    analysis.codec_guess = "h264";
    analysis.summary.frame_count = 1;
    analysis.summary.keyframe_count = 1;
    analysis.summary.sps_count = 2;
    analysis.summary.pps_count = 1;
    analysis.summary.gop_count = 1;

    streamview::analysis::NalAnalysis sps_nal_1{};
    sps_nal_1.index = 0;
    sps_nal_1.h264 = streamview::analysis::H264NalAnalysis{};
    sps_nal_1.h264->sps = streamview::bitstream::H264SpsInfo{};
    sps_nal_1.h264->sps->seq_parameter_set_id = 0;
    sps_nal_1.h264->sps->width = 640;
    sps_nal_1.h264->sps->height = 360;

    streamview::analysis::NalAnalysis sps_nal_2{};
    sps_nal_2.index = 1;
    sps_nal_2.h264 = streamview::analysis::H264NalAnalysis{};
    sps_nal_2.h264->sps = streamview::bitstream::H264SpsInfo{};
    sps_nal_2.h264->sps->seq_parameter_set_id = 1;
    sps_nal_2.h264->sps->width = 1280;
    sps_nal_2.h264->sps->height = 720;

    streamview::analysis::NalAnalysis slice_nal{};
    slice_nal.index = 2;
    slice_nal.h264 = streamview::analysis::H264NalAnalysis{};
    slice_nal.h264->slice = streamview::bitstream::H264SliceHeaderInfo{};
    slice_nal.h264->slice->pic_parameter_set_id = 0;

    analysis.nals = {sps_nal_1, sps_nal_2, slice_nal};
    analysis.frames.push_back({
        .index = 0,
        .decode_order_index = 0,
        .gop_index = 0,
        .codec = "h264",
        .frame_type = "I",
        .is_keyframe = true,
        .poc = std::nullopt,
        .nal_indices = {2},
        .size_bytes = 10,
        .first_payload_offset = 0,
    });
    analysis.gops.push_back({
        .index = 0,
        .start_frame_index = 0,
        .end_frame_index = 0,
        .frame_count = 1,
        .keyframe_index = 0,
        .size_bytes = 10,
        .starts_with_keyframe = true,
    });

    const auto issues = streamview::analysis::validate_stream_analysis(analysis);

    require(std::any_of(issues.begin(), issues.end(), [](const auto& issue) {
                return issue.code == "h264_resolution_change_detected" && issue.severity == "warning";
            }),
            "H.264 resolution change validation issue");
}

void test_validates_h265_resolution_change_detection() {
    streamview::analysis::StreamAnalysis analysis{};
    analysis.input_path = "resolution-change.h265";
    analysis.codec_guess = "h265";
    analysis.summary.frame_count = 1;
    analysis.summary.keyframe_count = 1;
    analysis.summary.vps_count = 1;
    analysis.summary.sps_count = 2;
    analysis.summary.pps_count = 1;
    analysis.summary.gop_count = 1;

    streamview::analysis::NalAnalysis vps_nal{};
    vps_nal.index = 0;
    vps_nal.h265 = streamview::analysis::H265NalAnalysis{};
    vps_nal.h265->vps = streamview::bitstream::H265VpsInfo{};
    vps_nal.h265->vps->video_parameter_set_id = 0;

    streamview::analysis::NalAnalysis sps_nal_1{};
    sps_nal_1.index = 1;
    sps_nal_1.h265 = streamview::analysis::H265NalAnalysis{};
    sps_nal_1.h265->sps = streamview::bitstream::H265SpsInfo{};
    sps_nal_1.h265->sps->seq_parameter_set_id = 0;
    sps_nal_1.h265->sps->video_parameter_set_id = 0;
    sps_nal_1.h265->sps->width = 640;
    sps_nal_1.h265->sps->height = 360;

    streamview::analysis::NalAnalysis sps_nal_2{};
    sps_nal_2.index = 2;
    sps_nal_2.h265 = streamview::analysis::H265NalAnalysis{};
    sps_nal_2.h265->sps = streamview::bitstream::H265SpsInfo{};
    sps_nal_2.h265->sps->seq_parameter_set_id = 1;
    sps_nal_2.h265->sps->video_parameter_set_id = 0;
    sps_nal_2.h265->sps->width = 1280;
    sps_nal_2.h265->sps->height = 720;

    streamview::analysis::NalAnalysis slice_nal{};
    slice_nal.index = 3;
    slice_nal.h265 = streamview::analysis::H265NalAnalysis{};
    slice_nal.h265->slice = streamview::bitstream::H265SliceHeaderInfo{};
    slice_nal.h265->slice->slice_pic_parameter_set_id = 0;

    analysis.nals = {vps_nal, sps_nal_1, sps_nal_2, slice_nal};
    analysis.frames.push_back({
        .index = 0,
        .decode_order_index = 0,
        .gop_index = 0,
        .codec = "h265",
        .frame_type = "I",
        .is_keyframe = true,
        .poc = std::nullopt,
        .nal_indices = {3},
        .size_bytes = 10,
        .first_payload_offset = 0,
    });
    analysis.gops.push_back({
        .index = 0,
        .start_frame_index = 0,
        .end_frame_index = 0,
        .frame_count = 1,
        .keyframe_index = 0,
        .size_bytes = 10,
        .starts_with_keyframe = true,
    });

    const auto issues = streamview::analysis::validate_stream_analysis(analysis);

    require(std::any_of(issues.begin(), issues.end(), [](const auto& issue) {
                return issue.code == "h265_resolution_change_detected" && issue.severity == "warning";
            }),
            "H.265 resolution change validation issue");
}

void test_records_h264_parse_errors_without_frames() {
    const std::vector<std::uint8_t> stream{
        0x00, 0x00, 0x01,
        0x67, 0x64,
        0x00, 0x00, 0x01,
        0x65, 0x88,
    };

    const auto analysis = streamview::analysis::analyze_h264_annex_b("bad.h264", stream);

    require(analysis.nals.size() == 2, "bad H.264 NAL count");
    require(analysis.summary.sps_count == 1, "bad H.264 SPS count");
    require(!analysis.summary.active_sps.has_value(), "bad H.264 active SPS absent");
    require(analysis.summary.frame_count == 0, "bad H.264 frame count");
    require(analysis.summary.slices.total == 0, "bad H.264 slice count");
    require(analysis.summary.parse_errors.total == 2, "bad H.264 parse error total");
    require(analysis.summary.parse_errors.sps == 1, "bad H.264 SPS parse errors");
    require(analysis.summary.parse_errors.slice == 1, "bad H.264 slice parse errors");
    require(analysis.nals[0].h264.has_value(), "bad H.264 SPS analysis present");
    require(analysis.nals[0].h264->sps_parse_error.has_value(), "bad H.264 SPS error");
    require(analysis.nals[1].h264.has_value(), "bad H.264 slice analysis present");
    require(analysis.nals[1].h264->slice_parse_error.has_value(), "bad H.264 missing SPS error");
}

void test_records_h265_parse_errors_without_crashing() {
    const std::vector<std::uint8_t> stream{
        0x00, 0x00, 0x01,
        0x40, 0x01,
        0x00, 0x00, 0x01,
        0x42, 0x01,
        0x00, 0x00, 0x01,
        0x44, 0x01,
        0x00, 0x00, 0x01,
        0x26, 0x01,
    };

    const auto analysis = streamview::analysis::analyze_h265_annex_b("bad.h265", stream);

    require(analysis.nals.size() == 4, "bad H.265 NAL count");
    require(analysis.summary.vps_count == 1, "bad H.265 VPS count");
    require(analysis.summary.sps_count == 1, "bad H.265 SPS count");
    require(analysis.summary.pps_count == 1, "bad H.265 PPS count");
    require(analysis.summary.frame_count == 0, "bad H.265 frame count");
    require(analysis.summary.slices.total == 0, "bad H.265 parsed slice count");
    require(analysis.summary.parse_errors.total == 4, "bad H.265 parse error total");
    require(analysis.summary.parse_errors.vps == 1, "bad H.265 VPS parse errors");
    require(analysis.summary.parse_errors.sps == 1, "bad H.265 SPS parse errors");
    require(analysis.summary.parse_errors.pps == 1, "bad H.265 PPS parse errors");
    require(analysis.summary.parse_errors.slice == 1, "bad H.265 slice parse errors");
    require(!analysis.summary.active_h265_sps.has_value(), "bad H.265 active SPS absent");
    require(analysis.nals[0].h265.has_value(), "bad H.265 VPS analysis present");
    require(analysis.nals[0].h265->vps_parse_error.has_value(), "bad H.265 VPS error");
    require(analysis.nals[1].h265.has_value(), "bad H.265 SPS analysis present");
    require(analysis.nals[1].h265->sps_parse_error.has_value(), "bad H.265 SPS error");
    require(analysis.nals[2].h265.has_value(), "bad H.265 PPS analysis present");
    require(analysis.nals[2].h265->pps_parse_error.has_value(), "bad H.265 PPS error");
    require(analysis.nals[3].h265.has_value(), "bad H.265 slice analysis present");
    require(analysis.nals[3].h265->slice_parse_error.has_value(), "bad H.265 slice error");
    require(analysis.frames.empty(), "bad H.265 frames empty");
}

} // namespace

int main() {
    test_analyzes_minimal_h264_stream();
    test_analyzes_h264_frame_poc_from_context();
    test_analyzes_minimal_h265_stream();
    test_analyzes_h265_frame_poc_from_context();
    test_validates_duplicate_h264_parameter_sets();
    test_validates_duplicate_h265_parameter_sets();
    test_validates_frame_gop_consistency();
    test_validates_resolution_change_detection();
    test_validates_h265_resolution_change_detection();
    test_records_h264_parse_errors_without_frames();
    test_records_h265_parse_errors_without_crashing();
    return 0;
}
