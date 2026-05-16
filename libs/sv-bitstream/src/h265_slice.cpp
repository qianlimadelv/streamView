#include "streamview/bitstream/h265_slice.hpp"

#include "streamview/bitstream/bit_reader.hpp"
#include "streamview/bitstream/rbsp.hpp"

namespace streamview::bitstream {
namespace {

[[nodiscard]] bool h265_nal_type_is_irap(H265NalType type) {
    const auto value = static_cast<std::uint8_t>(type);
    return value >= 16 && value <= 23;
}

[[nodiscard]] std::optional<H265SliceKind> to_slice_kind(std::uint32_t slice_type_raw) {
    switch (slice_type_raw) {
    case 0:
        return H265SliceKind::B;
    case 1:
        return H265SliceKind::P;
    case 2:
        return H265SliceKind::I;
    default:
        return std::nullopt;
    }
}

} // namespace

H265SliceHeaderParseResult parse_h265_slice_header(
    std::span<const std::uint8_t> nal_payload,
    H265NalType nal_unit_type,
    std::optional<std::uint8_t> num_extra_slice_header_bits) {
    return parse_h265_slice_header(
        nal_payload,
        nal_unit_type,
        H265SliceHeaderContext{
            .num_extra_slice_header_bits = num_extra_slice_header_bits,
            .is_irap = h265_nal_type_is_irap(nal_unit_type),
        });
}

H265SliceHeaderParseResult parse_h265_slice_header(
    std::span<const std::uint8_t> nal_payload,
    H265NalType nal_unit_type,
    const H265SliceHeaderContext& context) {
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

    if (!info.first_slice_segment_in_pic_flag && context.dependent_slice_segments_enabled_flag) {
        const auto dependent_slice_segment_flag = reader.read_bit();
        if (!dependent_slice_segment_flag.has_value()) {
            return {Status::parse_error("failed to read H.265 dependent_slice_segment_flag"), std::nullopt};
        }
        info.dependent_slice_segment_flag_present = true;
        info.dependent_slice_segment_flag = *dependent_slice_segment_flag;
    }

    if (info.first_slice_segment_in_pic_flag && context.num_extra_slice_header_bits.has_value()) {
        if (!reader.read_bits(*context.num_extra_slice_header_bits).has_value()) {
            return {Status::parse_error("failed to read H.265 extra slice header bits"), std::nullopt};
        }

        const auto slice_type_raw = reader.read_ue();
        if (!slice_type_raw.has_value()) {
            return {Status::parse_error("failed to read H.265 slice_type"), std::nullopt};
        }
        const auto slice_kind = to_slice_kind(*slice_type_raw);
        if (!slice_kind.has_value()) {
            return {Status::parse_error("invalid H.265 slice_type"), std::nullopt};
        }
        info.slice_type_present = true;
        info.slice_type_raw = *slice_type_raw;
        info.slice_kind = *slice_kind;

        if (context.output_flag_present_flag) {
            const auto pic_output_flag = reader.read_bit();
            if (!pic_output_flag.has_value()) {
                return {Status::parse_error("failed to read H.265 pic_output_flag"), std::nullopt};
            }
            info.pic_output_flag_present = true;
            info.pic_output_flag = *pic_output_flag;
        }
    }

    if (!context.is_irap && context.log2_max_pic_order_cnt_lsb_minus4.has_value()) {
        const auto pic_order_cnt_lsb = reader.read_bits(*context.log2_max_pic_order_cnt_lsb_minus4 + 4);
        if (!pic_order_cnt_lsb.has_value()) {
            return {Status::parse_error("failed to read H.265 slice_pic_order_cnt_lsb"), std::nullopt};
        }
        info.pic_order_cnt_lsb_present = true;
        info.pic_order_cnt_lsb = *pic_order_cnt_lsb;
    }

    return {Status::ok(), info};
}

std::string_view h265_slice_kind_name(H265SliceKind kind) {
    switch (kind) {
    case H265SliceKind::B:
        return "B";
    case H265SliceKind::P:
        return "P";
    case H265SliceKind::I:
        return "I";
    }
    return "unknown";
}

} // namespace streamview::bitstream
