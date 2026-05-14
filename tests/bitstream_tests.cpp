#include "streamview/bitstream/annex_b.hpp"
#include "streamview/bitstream/h264_nal.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

namespace {

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

} // namespace

int main() {
    test_scans_three_and_four_byte_start_codes();
    test_ignores_leading_bytes_and_empty_units();
    test_returns_empty_without_start_code();
    test_parses_h264_nal_header();
    return 0;
}
