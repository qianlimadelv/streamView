#pragma once

#include <cstdint>
#include <string_view>

namespace streamview::bitstream {

enum class H264NalType : std::uint8_t {
    Unspecified = 0,
    CodedSliceNonIdr = 1,
    CodedSliceDataPartitionA = 2,
    CodedSliceDataPartitionB = 3,
    CodedSliceDataPartitionC = 4,
    CodedSliceIdr = 5,
    Sei = 6,
    Sps = 7,
    Pps = 8,
    Aud = 9,
    EndOfSequence = 10,
    EndOfStream = 11,
    FillerData = 12,
};

struct H264NalHeader {
    std::uint8_t forbidden_zero_bit{};
    std::uint8_t nal_ref_idc{};
    H264NalType nal_unit_type{H264NalType::Unspecified};
};

[[nodiscard]] H264NalHeader parse_h264_nal_header(std::uint8_t byte);
[[nodiscard]] std::string_view h264_nal_type_name(H264NalType type);

} // namespace streamview::bitstream
