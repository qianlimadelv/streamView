#include "streamview/bitstream/h264_nal.hpp"

namespace streamview::bitstream {

H264NalHeader parse_h264_nal_header(std::uint8_t byte) {
    return {
        .forbidden_zero_bit = static_cast<std::uint8_t>((byte >> 7) & 0x01),
        .nal_ref_idc = static_cast<std::uint8_t>((byte >> 5) & 0x03),
        .nal_unit_type = static_cast<H264NalType>(byte & 0x1F),
    };
}

std::string_view h264_nal_type_name(H264NalType type) {
    switch (type) {
    case H264NalType::Unspecified:
        return "unspecified";
    case H264NalType::CodedSliceNonIdr:
        return "coded_slice_non_idr";
    case H264NalType::CodedSliceDataPartitionA:
        return "coded_slice_data_partition_a";
    case H264NalType::CodedSliceDataPartitionB:
        return "coded_slice_data_partition_b";
    case H264NalType::CodedSliceDataPartitionC:
        return "coded_slice_data_partition_c";
    case H264NalType::CodedSliceIdr:
        return "coded_slice_idr";
    case H264NalType::Sei:
        return "sei";
    case H264NalType::Sps:
        return "sps";
    case H264NalType::Pps:
        return "pps";
    case H264NalType::Aud:
        return "aud";
    case H264NalType::EndOfSequence:
        return "end_of_sequence";
    case H264NalType::EndOfStream:
        return "end_of_stream";
    case H264NalType::FillerData:
        return "filler_data";
    default:
        return "reserved_or_unspecified";
    }
}

} // namespace streamview::bitstream
