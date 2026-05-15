#include "streamview/bitstream/h265_pps.hpp"

#include "streamview/bitstream/bit_reader.hpp"
#include "streamview/bitstream/rbsp.hpp"

namespace streamview::bitstream {

H265PpsParseResult parse_h265_pps(std::span<const std::uint8_t> nal_payload) {
    if (nal_payload.size() < 3) {
        return {Status::parse_error("H.265 PPS NAL payload is too small"), std::nullopt};
    }

    const auto rbsp = nal_payload_to_rbsp(nal_payload);
    BitReader reader(std::span<const std::uint8_t>(rbsp).subspan(2));

    H265PpsInfo info{};
    const auto pic_parameter_set_id = reader.read_ue();
    const auto seq_parameter_set_id = reader.read_ue();
    const auto dependent_slice_segments_enabled_flag = reader.read_bit();
    const auto output_flag_present_flag = reader.read_bit();
    const auto num_extra_slice_header_bits = reader.read_bits(3);
    if (!pic_parameter_set_id.has_value() || !seq_parameter_set_id.has_value() ||
        !dependent_slice_segments_enabled_flag.has_value() || !output_flag_present_flag.has_value() ||
        !num_extra_slice_header_bits.has_value()) {
        return {Status::parse_error("failed to read H.265 PPS baseline fields"), std::nullopt};
    }

    info.pic_parameter_set_id = *pic_parameter_set_id;
    info.seq_parameter_set_id = *seq_parameter_set_id;
    info.dependent_slice_segments_enabled_flag = *dependent_slice_segments_enabled_flag;
    info.output_flag_present_flag = *output_flag_present_flag;
    info.num_extra_slice_header_bits = static_cast<std::uint8_t>(*num_extra_slice_header_bits);

    return {Status::ok(), info};
}

} // namespace streamview::bitstream
