#include "streamview/bitstream/h265_sps.hpp"

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
                                            H265SpsInfo& info) {
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

[[nodiscard]] std::uint32_t chroma_sub_width(std::uint32_t chroma_format_idc) {
    if (chroma_format_idc == 1 || chroma_format_idc == 2) {
        return 2;
    }
    return 1;
}

[[nodiscard]] std::uint32_t chroma_sub_height(std::uint32_t chroma_format_idc) {
    if (chroma_format_idc == 1) {
        return 2;
    }
    return 1;
}

} // namespace

H265SpsParseResult parse_h265_sps(std::span<const std::uint8_t> nal_payload) {
    if (nal_payload.size() < 3) {
        return {Status::parse_error("H.265 SPS NAL payload is too small"), std::nullopt};
    }

    const auto rbsp = nal_payload_to_rbsp(nal_payload);
    BitReader reader(std::span<const std::uint8_t>(rbsp).subspan(2));

    H265SpsInfo info{};
    const auto video_parameter_set_id = reader.read_bits(4);
    const auto max_sub_layers_minus1 = reader.read_bits(3);
    const auto temporal_id_nesting_flag = reader.read_bit();
    if (!video_parameter_set_id.has_value() || !max_sub_layers_minus1.has_value() ||
        !temporal_id_nesting_flag.has_value()) {
        return {Status::parse_error("failed to read H.265 SPS header"), std::nullopt};
    }

    info.video_parameter_set_id = *video_parameter_set_id;
    info.max_sub_layers_minus1 = *max_sub_layers_minus1;
    if (!parse_profile_tier_level(reader, info.max_sub_layers_minus1, info)) {
        return {Status::parse_error("failed to read H.265 profile_tier_level"), std::nullopt};
    }

    const auto seq_parameter_set_id = reader.read_ue();
    const auto chroma_format_idc = reader.read_ue();
    if (!seq_parameter_set_id.has_value() || !chroma_format_idc.has_value()) {
        return {Status::parse_error("failed to read H.265 SPS ids"), std::nullopt};
    }
    if (*chroma_format_idc > 3) {
        return {Status::parse_error("invalid H.265 chroma_format_idc"), std::nullopt};
    }

    info.seq_parameter_set_id = *seq_parameter_set_id;
    info.chroma_format_idc = *chroma_format_idc;

    if (info.chroma_format_idc == 3) {
        const auto separate_colour_plane_flag = reader.read_bit();
        if (!separate_colour_plane_flag.has_value()) {
            return {Status::parse_error("failed to read H.265 separate_colour_plane_flag"), std::nullopt};
        }
        info.separate_colour_plane_flag = *separate_colour_plane_flag;
    }

    const auto pic_width_in_luma_samples = reader.read_ue();
    const auto pic_height_in_luma_samples = reader.read_ue();
    if (!pic_width_in_luma_samples.has_value() || !pic_height_in_luma_samples.has_value()) {
        return {Status::parse_error("failed to read H.265 SPS dimensions"), std::nullopt};
    }
    info.width = *pic_width_in_luma_samples;
    info.height = *pic_height_in_luma_samples;

    const auto conformance_window_flag = reader.read_bit();
    if (!conformance_window_flag.has_value()) {
        return {Status::parse_error("failed to read H.265 conformance_window_flag"), std::nullopt};
    }
    info.conformance_window_flag = *conformance_window_flag;
    if (info.conformance_window_flag) {
        const auto left = reader.read_ue();
        const auto right = reader.read_ue();
        const auto top = reader.read_ue();
        const auto bottom = reader.read_ue();
        if (!left.has_value() || !right.has_value() || !top.has_value() || !bottom.has_value()) {
            return {Status::parse_error("failed to read H.265 conformance window"), std::nullopt};
        }

        info.conf_win_left_offset = *left;
        info.conf_win_right_offset = *right;
        info.conf_win_top_offset = *top;
        info.conf_win_bottom_offset = *bottom;

        const std::uint32_t crop_width =
            (info.conf_win_left_offset + info.conf_win_right_offset) * chroma_sub_width(info.chroma_format_idc);
        const std::uint32_t crop_height =
            (info.conf_win_top_offset + info.conf_win_bottom_offset) * chroma_sub_height(info.chroma_format_idc);
        if (crop_width >= info.width || crop_height >= info.height) {
            return {Status::parse_error("invalid H.265 SPS conformance window"), std::nullopt};
        }
        info.width -= crop_width;
        info.height -= crop_height;
    }

    const auto bit_depth_luma_minus8 = reader.read_ue();
    const auto bit_depth_chroma_minus8 = reader.read_ue();
    if (!bit_depth_luma_minus8.has_value() || !bit_depth_chroma_minus8.has_value()) {
        return {Status::parse_error("failed to read H.265 bit depth"), std::nullopt};
    }
    info.bit_depth_luma = static_cast<std::uint8_t>(*bit_depth_luma_minus8 + 8);
    info.bit_depth_chroma = static_cast<std::uint8_t>(*bit_depth_chroma_minus8 + 8);

    const auto log2_max_pic_order_cnt_lsb_minus4 = reader.read_ue();
    if (!log2_max_pic_order_cnt_lsb_minus4.has_value()) {
        return {Status::parse_error("failed to read H.265 log2_max_pic_order_cnt_lsb_minus4"), std::nullopt};
    }
    if (*log2_max_pic_order_cnt_lsb_minus4 > 12) {
        return {Status::parse_error("invalid H.265 log2_max_pic_order_cnt_lsb_minus4"), std::nullopt};
    }
    info.log2_max_pic_order_cnt_lsb_minus4 = *log2_max_pic_order_cnt_lsb_minus4;

    return {Status::ok(), info};
}

} // namespace streamview::bitstream
