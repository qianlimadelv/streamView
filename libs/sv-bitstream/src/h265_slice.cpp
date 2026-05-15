#include "streamview/bitstream/h265_slice.hpp"

#include "streamview/bitstream/bit_reader.hpp"
#include "streamview/bitstream/rbsp.hpp"

namespace streamview::bitstream {
namespace {

[[nodiscard]] bool h265_nal_type_is_irap(H265NalType type) {
    const auto value = static_cast<std::uint8_t>(type);
    return value >= 16 && value <= 23;
}

} // namespace

H265SliceHeaderParseResult parse_h265_slice_header(
    std::span<const std::uint8_t> nal_payload,
    H265NalType nal_unit_type) {
    if (nal_payload.size() < 3) {
        return {Status::parse_error("H.265 slice NAL payload is too small"), std::nullopt};
    }

    const auto rbsp = nal_payload_to_rbsp(nal_payload);
    BitReader reader(std::span<const std::uint8_t>(rbsp).subspan(2));

    H265SliceHeaderInfo info{};
    const auto first_slice_segment_in_pic_flag = reader.read_bit();
    if (!first_slice_segment_in_pic_flag.has_value()) {
        return {Status::parse_error("failed to read H.265 first_slice_segment_in_pic_flag"), std::nullopt};
    }
    info.first_slice_segment_in_pic_flag = *first_slice_segment_in_pic_flag;

    if (h265_nal_type_is_irap(nal_unit_type)) {
        const auto no_output_of_prior_pics_flag = reader.read_bit();
        if (!no_output_of_prior_pics_flag.has_value()) {
            return {Status::parse_error("failed to read H.265 no_output_of_prior_pics_flag"), std::nullopt};
        }
        info.no_output_of_prior_pics_flag = *no_output_of_prior_pics_flag;
        info.no_output_of_prior_pics_flag_present = true;
    }

    const auto slice_pic_parameter_set_id = reader.read_ue();
    if (!slice_pic_parameter_set_id.has_value()) {
        return {Status::parse_error("failed to read H.265 slice_pic_parameter_set_id"), std::nullopt};
    }
    info.slice_pic_parameter_set_id = *slice_pic_parameter_set_id;

    return {Status::ok(), info};
}

} // namespace streamview::bitstream
