#include "streamview/bitstream/h264_pps.hpp"

#include "streamview/bitstream/bit_reader.hpp"
#include "streamview/bitstream/rbsp.hpp"

namespace streamview::bitstream {

H264PpsParseResult parse_h264_pps(std::span<const std::uint8_t> nal_payload) {
    if (nal_payload.size() < 2) {
        return {Status::parse_error("H.264 PPS NAL payload is too small"), std::nullopt};
    }

    const auto rbsp = nal_payload_to_rbsp(nal_payload);
    BitReader reader(std::span<const std::uint8_t>(rbsp).subspan(1));

    H264PpsInfo info{};
    const auto pic_parameter_set_id = reader.read_ue();
    const auto seq_parameter_set_id = reader.read_ue();
    const auto entropy_coding_mode_flag = reader.read_bit();
    const auto bottom_field_pic_order_in_frame_present_flag = reader.read_bit();
    const auto num_slice_groups_minus1 = reader.read_ue();

    if (!pic_parameter_set_id.has_value() || !seq_parameter_set_id.has_value() ||
        !entropy_coding_mode_flag.has_value() || !bottom_field_pic_order_in_frame_present_flag.has_value() ||
        !num_slice_groups_minus1.has_value()) {
        return {Status::parse_error("failed to read H.264 PPS baseline fields"), std::nullopt};
    }

    info.pic_parameter_set_id = *pic_parameter_set_id;
    info.seq_parameter_set_id = *seq_parameter_set_id;
    info.entropy_coding_mode_flag = *entropy_coding_mode_flag;
    info.bottom_field_pic_order_in_frame_present_flag = *bottom_field_pic_order_in_frame_present_flag;
    info.num_slice_groups_minus1 = *num_slice_groups_minus1;

    return {Status::ok(), info};
}

} // namespace streamview::bitstream
