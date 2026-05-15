#include "streamview/bitstream/h265_vps.hpp"

#include "streamview/bitstream/bit_reader.hpp"
#include "streamview/bitstream/rbsp.hpp"

#include <array>

namespace streamview::bitstream {
namespace {

[[nodiscard]] bool skip_bits(BitReader& reader, std::size_t count) {
    while (count > 0) {
        const std::size_t chunk = count > 32 ? 32 : count;
        if (!reader.read_bits(chunk).has_value()) {
            return false;
        }
        count -= chunk;
    }
    return true;
}

[[nodiscard]] bool parse_profile_tier_level(BitReader& reader,
                                            std::uint32_t max_sub_layers_minus1,
                                            H265VpsInfo& info) {
    if (max_sub_layers_minus1 > 6) {
        return false;
    }

    if (!reader.read_bits(2).has_value()) {
        return false;
    }
    const auto tier_flag = reader.read_bit();
    const auto profile_idc = reader.read_bits(5);
    if (!tier_flag.has_value() || !profile_idc.has_value()) {
        return false;
    }
    info.tier_flag = *tier_flag;
    info.profile_idc = static_cast<std::uint8_t>(*profile_idc);

    if (!skip_bits(reader, 32) || !skip_bits(reader, 48)) {
        return false;
    }

    const auto level_idc = reader.read_bits(8);
    if (!level_idc.has_value()) {
        return false;
    }
    info.level_idc = static_cast<std::uint8_t>(*level_idc);

    std::array<bool, 6> sub_layer_profile_present{};
    std::array<bool, 6> sub_layer_level_present{};
    for (std::uint32_t i = 0; i < max_sub_layers_minus1; ++i) {
        const auto profile_present = reader.read_bit();
        const auto level_present = reader.read_bit();
        if (!profile_present.has_value() || !level_present.has_value()) {
            return false;
        }
        sub_layer_profile_present[i] = *profile_present;
        sub_layer_level_present[i] = *level_present;
    }

    if (max_sub_layers_minus1 > 0) {
        for (std::uint32_t i = max_sub_layers_minus1; i < 8; ++i) {
            if (!reader.read_bits(2).has_value()) {
                return false;
            }
        }
    }

    for (std::uint32_t i = 0; i < max_sub_layers_minus1; ++i) {
        if (sub_layer_profile_present[i] && (!skip_bits(reader, 2) || !reader.read_bit().has_value() ||
                                             !skip_bits(reader, 5) || !skip_bits(reader, 32) ||
                                             !skip_bits(reader, 48))) {
            return false;
        }
        if (sub_layer_level_present[i] && !skip_bits(reader, 8)) {
            return false;
        }
    }

    return true;
}

} // namespace

H265VpsParseResult parse_h265_vps(std::span<const std::uint8_t> nal_payload) {
    if (nal_payload.size() < 3) {
        return {Status::parse_error("H.265 VPS NAL payload is too small"), std::nullopt};
    }

    const auto rbsp = nal_payload_to_rbsp(nal_payload);
    BitReader reader(std::span<const std::uint8_t>(rbsp).subspan(2));

    H265VpsInfo info{};
    const auto video_parameter_set_id = reader.read_bits(4);
    const auto base_layer_internal_flag = reader.read_bit();
    const auto base_layer_available_flag = reader.read_bit();
    const auto max_layers_minus1 = reader.read_bits(6);
    const auto max_sub_layers_minus1 = reader.read_bits(3);
    const auto temporal_id_nesting_flag = reader.read_bit();
    const auto reserved = reader.read_bits(16);
    if (!video_parameter_set_id.has_value() || !base_layer_internal_flag.has_value() ||
        !base_layer_available_flag.has_value() || !max_layers_minus1.has_value() ||
        !max_sub_layers_minus1.has_value() || !temporal_id_nesting_flag.has_value() || !reserved.has_value()) {
        return {Status::parse_error("failed to read H.265 VPS header"), std::nullopt};
    }
    if (*reserved != 0xffff) {
        return {Status::parse_error("invalid H.265 VPS reserved bits"), std::nullopt};
    }

    info.video_parameter_set_id = *video_parameter_set_id;
    info.base_layer_internal_flag = *base_layer_internal_flag;
    info.base_layer_available_flag = *base_layer_available_flag;
    info.max_layers_minus1 = *max_layers_minus1;
    info.max_sub_layers_minus1 = *max_sub_layers_minus1;
    info.temporal_id_nesting_flag = *temporal_id_nesting_flag;

    if (!parse_profile_tier_level(reader, info.max_sub_layers_minus1, info)) {
        return {Status::parse_error("failed to read H.265 VPS profile_tier_level"), std::nullopt};
    }

    const auto sub_layer_ordering_info_present_flag = reader.read_bit();
    if (!sub_layer_ordering_info_present_flag.has_value()) {
        return {Status::parse_error("failed to read H.265 VPS ordering flag"), std::nullopt};
    }

    const std::uint32_t first_ordering_layer = *sub_layer_ordering_info_present_flag ? 0 : info.max_sub_layers_minus1;
    for (std::uint32_t i = first_ordering_layer; i <= info.max_sub_layers_minus1; ++i) {
        const auto max_dec_pic_buffering_minus1 = reader.read_ue();
        const auto max_num_reorder_pics = reader.read_ue();
        const auto max_latency_increase_plus1 = reader.read_ue();
        if (!max_dec_pic_buffering_minus1.has_value() || !max_num_reorder_pics.has_value() ||
            !max_latency_increase_plus1.has_value()) {
            return {Status::parse_error("failed to read H.265 VPS sub-layer ordering info"), std::nullopt};
        }
        if (i == info.max_sub_layers_minus1) {
            info.max_dec_pic_buffering_minus1 = *max_dec_pic_buffering_minus1;
            info.max_num_reorder_pics = *max_num_reorder_pics;
            info.max_latency_increase_plus1 = *max_latency_increase_plus1;
        }
    }

    const auto max_layer_id = reader.read_bits(6);
    const auto num_layer_sets_minus1 = reader.read_ue();
    if (!max_layer_id.has_value() || !num_layer_sets_minus1.has_value()) {
        return {Status::parse_error("failed to read H.265 VPS layer set fields"), std::nullopt};
    }
    info.max_layer_id = *max_layer_id;
    info.num_layer_sets_minus1 = *num_layer_sets_minus1;

    return {Status::ok(), info};
}

} // namespace streamview::bitstream
