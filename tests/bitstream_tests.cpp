#include "streamview/bitstream/annex_b.hpp"
#include "streamview/bitstream/bit_reader.hpp"
#include "streamview/bitstream/h264_nal.hpp"
#include "streamview/bitstream/h264_pps.hpp"
#include "streamview/bitstream/h264_slice.hpp"
#include "streamview/bitstream/h264_sps.hpp"
#include "streamview/bitstream/h265_nal.hpp"
#include "streamview/bitstream/h265_sps.hpp"
#include "streamview/bitstream/rbsp.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <span>
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
    writer.write_bits(0, 4);   // sps_video_parameter_set_id
    writer.write_bits(0, 3);   // sps_max_sub_layers_minus1
    writer.write_bit(true);    // sps_temporal_id_nesting_flag
    writer.write_bits(0, 2);   // general_profile_space
    writer.write_bit(false);   // general_tier_flag
    writer.write_bits(1, 5);   // general_profile_idc
    writer.write_bits(0, 32);  // compatibility flags
    writer.write_bits(0, 32);  // constraint flags high bits
    writer.write_bits(0, 16);  // constraint flags low bits
    writer.write_bits(120, 8); // general_level_idc
    writer.write_ue(0);        // sps_seq_parameter_set_id
    writer.write_ue(1);        // chroma_format_idc
    writer.write_ue(width);
    writer.write_ue(height);
    writer.write_bit(false); // conformance_window_flag
    writer.write_ue(0);      // bit_depth_luma_minus8
    writer.write_ue(0);      // bit_depth_chroma_minus8

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

void test_scans_three_and_four_byte_start_codes() {
    const std::vector<std::uint8_t> data{
        0x00, 0x00, 0x01, 0x67, 0x11, 0x22,
        0x00, 0x00, 0x00, 0x01, 0x68, 0x33,
    };

    const auto units = streamview::bitstream::scan_annex_b(data);

    require(units.size() == 2, "expected two NAL units");
    require(units[0].start_code_offset == 0, "first start code offset");
    require(units[0].start_code_size == 3, "first start code size");
    require(units[0].payload_offset == 3, "first payload offset");
    require(units[0].payload_size == 3, "first payload size");
    require(units[1].start_code_offset == 6, "second start code offset");
    require(units[1].start_code_size == 4, "second start code size");
    require(units[1].payload_offset == 10, "second payload offset");
    require(units[1].payload_size == 2, "second payload size");
}

void test_ignores_leading_bytes_and_empty_units() {
    const std::vector<std::uint8_t> data{
        0xAA, 0xBB,
        0x00, 0x00, 0x01,
        0x00, 0x00, 0x01, 0x65, 0x88,
    };

    const auto units = streamview::bitstream::scan_annex_b(data);

    require(units.size() == 1, "expected one non-empty NAL unit");
    require(units[0].start_code_offset == 5, "non-empty unit start offset");
    require(units[0].payload_offset == 8, "non-empty unit payload offset");
    require(units[0].payload_size == 2, "non-empty unit payload size");
}

void test_returns_empty_without_start_code() {
    const std::vector<std::uint8_t> data{0x12, 0x34, 0x56};
    const auto units = streamview::bitstream::scan_annex_b(data);
    require(units.empty(), "expected no NAL units");
}

void test_parses_h264_nal_header() {
    const auto header = streamview::bitstream::parse_h264_nal_header(0x65);
    require(header.forbidden_zero_bit == 0, "forbidden_zero_bit");
    require(header.nal_ref_idc == 3, "nal_ref_idc");
    require(header.nal_unit_type == streamview::bitstream::H264NalType::CodedSliceIdr, "nal_unit_type");
    require(streamview::bitstream::h264_nal_type_name(header.nal_unit_type) == "coded_slice_idr", "nal type name");
}

void test_removes_emulation_prevention_bytes() {
    const std::vector<std::uint8_t> payload{
        0x67, 0x00, 0x00, 0x03, 0x01, 0xAA,
        0x00, 0x00, 0x03, 0x02, 0xBB,
    };

    const auto rbsp = streamview::bitstream::nal_payload_to_rbsp(payload);

    const std::vector<std::uint8_t> expected{
        0x67, 0x00, 0x00, 0x01, 0xAA,
        0x00, 0x00, 0x02, 0xBB,
    };
    require(rbsp == expected, "rbsp extraction should remove emulation prevention bytes");
}

void test_bit_reader_reads_across_byte_boundaries() {
    const std::vector<std::uint8_t> data{0b1010'1100, 0b0111'0000};
    streamview::bitstream::BitReader reader(data);

    const auto first = reader.read_bits(4);
    const auto second = reader.read_bits(6);
    const auto third = reader.read_bits(2);

    require(first.has_value() && *first == 0b1010, "first nibble");
    require(second.has_value() && *second == 0b110001, "cross-byte read");
    require(third.has_value() && *third == 0b11, "third read");
    require(reader.bit_offset() == 12, "bit offset after reads");
}

void test_bit_reader_decodes_exp_golomb() {
    const std::vector<std::uint8_t> data{0b1010'0011, 0b0011'0000};
    streamview::bitstream::BitReader reader(data);

    const auto zero = reader.read_ue();      // 1
    const auto one = reader.read_ue();       // 010
    const auto five = reader.read_ue();      // 00110
    const auto signed_minus_one = reader.read_se(); // 011 -> code_num 2 -> -1

    require(zero.has_value() && *zero == 0, "ue zero");
    require(one.has_value() && *one == 1, "ue one");
    require(five.has_value() && *five == 5, "ue five");
    require(signed_minus_one.has_value() && *signed_minus_one == -1, "se minus one");
}

void test_parses_h264_sps_dimensions() {
    const std::vector<std::uint8_t> sps{
        0x67, 0x64, 0x00, 0x1f, 0xac, 0xd9, 0x40, 0xa0,
        0x2f, 0xf9, 0x70, 0x11, 0x00, 0x00, 0x03, 0x00,
        0x01, 0x00, 0x00, 0x03, 0x00, 0x32, 0x0f, 0x18,
        0x31, 0x96,
    };

    const auto result = streamview::bitstream::parse_h264_sps(sps);

    require(result.status.is_ok(), "SPS parse status");
    require(result.info.has_value(), "SPS info present");
    require(result.info->profile_idc == 100, "SPS profile_idc");
    require(result.info->level_idc == 31, "SPS level_idc");
    require(result.info->chroma_format_idc == 1, "SPS chroma_format_idc");
    require(result.info->bit_depth_luma == 8, "SPS bit_depth_luma");
    require(result.info->bit_depth_chroma == 8, "SPS bit_depth_chroma");
    require(result.info->log2_max_frame_num_minus4 == 0, "SPS log2_max_frame_num_minus4");
    require(result.info->width == 640, "SPS width");
    require(result.info->height == 360, "SPS height");
}

void test_parses_h264_pps_baseline_fields() {
    const std::vector<std::uint8_t> pps{0x68, 0xeb, 0xec, 0xb2};

    const auto result = streamview::bitstream::parse_h264_pps(pps);

    require(result.status.is_ok(), "PPS parse status");
    require(result.info.has_value(), "PPS info present");
    require(result.info->pic_parameter_set_id == 0, "PPS pic_parameter_set_id");
    require(result.info->seq_parameter_set_id == 0, "PPS seq_parameter_set_id");
    require(result.info->entropy_coding_mode_flag, "PPS entropy_coding_mode_flag");
    require(!result.info->bottom_field_pic_order_in_frame_present_flag, "PPS bottom field flag");
    require(result.info->num_slice_groups_minus1 == 0, "PPS num_slice_groups_minus1");
}

void test_parses_h264_slice_header_prefix() {
    const std::vector<std::uint8_t> slice{0x65, 0x88, 0x80};

    const auto result = streamview::bitstream::parse_h264_slice_header(slice, 0);

    require(result.status.is_ok(), "slice parse status");
    require(result.info.has_value(), "slice info present");
    require(result.info->first_mb_in_slice == 0, "slice first_mb_in_slice");
    require(result.info->slice_type_raw == 7, "slice_type_raw");
    require(result.info->slice_kind == streamview::bitstream::H264SliceKind::I, "slice kind");
    require(result.info->slice_type_all_slices, "slice_type_all_slices");
    require(result.info->pic_parameter_set_id == 0, "slice pps id");
    require(result.info->frame_num == 0, "slice frame_num");
    require(streamview::bitstream::h264_slice_kind_name(result.info->slice_kind) == "I", "slice kind name");
}

void test_parses_h265_nal_header() {
    const auto vps = streamview::bitstream::parse_h265_nal_header(0x40, 0x01);
    require(vps.forbidden_zero_bit == 0, "H.265 VPS forbidden_zero_bit");
    require(vps.nal_unit_type == streamview::bitstream::H265NalType::Vps, "H.265 VPS nal_unit_type");
    require(vps.nuh_layer_id == 0, "H.265 VPS nuh_layer_id");
    require(vps.nuh_temporal_id_plus1 == 1, "H.265 VPS temporal id");
    require(streamview::bitstream::h265_nal_type_name(vps.nal_unit_type) == "vps", "H.265 VPS type name");

    const auto idr = streamview::bitstream::parse_h265_nal_header(0x26, 0x01);
    require(idr.nal_unit_type == streamview::bitstream::H265NalType::IdrWRadl, "H.265 IDR nal_unit_type");
    require(streamview::bitstream::h265_nal_type_is_vcl(idr.nal_unit_type), "H.265 IDR is VCL");
}

void test_parses_h265_sps_dimensions() {
    const auto sps = make_h265_sps_payload(640, 360);

    const auto result = streamview::bitstream::parse_h265_sps(sps);

    require(result.status.is_ok(), "H.265 SPS parse status");
    require(result.info.has_value(), "H.265 SPS info present");
    require(result.info->profile_idc == 1, "H.265 SPS profile_idc");
    require(result.info->level_idc == 120, "H.265 SPS level_idc");
    require(result.info->video_parameter_set_id == 0, "H.265 SPS vps id");
    require(result.info->seq_parameter_set_id == 0, "H.265 SPS sps id");
    require(result.info->chroma_format_idc == 1, "H.265 SPS chroma_format_idc");
    require(result.info->bit_depth_luma == 8, "H.265 SPS bit_depth_luma");
    require(result.info->bit_depth_chroma == 8, "H.265 SPS bit_depth_chroma");
    require(result.info->width == 640, "H.265 SPS width");
    require(result.info->height == 360, "H.265 SPS height");
}

} // namespace

int main() {
    test_scans_three_and_four_byte_start_codes();
    test_ignores_leading_bytes_and_empty_units();
    test_returns_empty_without_start_code();
    test_parses_h264_nal_header();
    test_removes_emulation_prevention_bytes();
    test_bit_reader_reads_across_byte_boundaries();
    test_bit_reader_decodes_exp_golomb();
    test_parses_h264_sps_dimensions();
    test_parses_h264_pps_baseline_fields();
    test_parses_h264_slice_header_prefix();
    test_parses_h265_nal_header();
    test_parses_h265_sps_dimensions();
    return 0;
}
