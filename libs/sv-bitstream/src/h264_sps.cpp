#include "streamview/bitstream/h264_sps.hpp"

#include "streamview/bitstream/bit_reader.hpp"
#include "streamview/bitstream/rbsp.hpp"

#include <array>

namespace streamview::bitstream {
namespace {

[[nodiscard]] bool is_high_profile(std::uint8_t profile_idc) {
    constexpr std::array<std::uint8_t, 11> high_profiles{
        100, 110, 122, 244, 44, 83, 86, 118, 128, 138, 144,
    };
    for (const auto profile : high_profiles) {
        if (profile == profile_idc) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool skip_scaling_list(BitReader& reader, std::size_t size) {
    int last_scale = 8;
    int next_scale = 8;
    for (std::size_t j = 0; j < size; ++j) {
        if (next_scale != 0) {
            const auto delta_scale = reader.read_se();
            if (!delta_scale.has_value()) {
                return false;
            }
            next_scale = (last_scale + *delta_scale + 256) % 256;
        }
        last_scale = next_scale == 0 ? last_scale : next_scale;
    }
    return true;
}

[[nodiscard]] bool skip_hrd_parameters(BitReader& reader) {
    const auto cpb_cnt_minus1 = reader.read_ue();
    if (!cpb_cnt_minus1.has_value()) {
        return false;
    }
    if (!reader.read_bits(4).has_value() || !reader.read_bits(4).has_value()) {
        return false;
    }
    for (std::uint32_t i = 0; i <= *cpb_cnt_minus1; ++i) {
        if (!reader.read_ue().has_value() || !reader.read_ue().has_value() || !reader.read_bit().has_value()) {
            return false;
        }
    }
    return reader.read_bits(5).has_value() && reader.read_bits(5).has_value() &&
           reader.read_bits(5).has_value() && reader.read_bits(5).has_value();
}

[[nodiscard]] bool parse_vui_parameters(BitReader& reader, H264VuiInfo& vui) {
    const auto aspect_ratio_info_present_flag = reader.read_bit();
    if (!aspect_ratio_info_present_flag.has_value()) {
        return false;
    }
    vui.aspect_ratio_info_present_flag = *aspect_ratio_info_present_flag;
    if (*aspect_ratio_info_present_flag) {
        const auto aspect_ratio_idc = reader.read_bits(8);
        if (!aspect_ratio_idc.has_value()) {
            return false;
        }
        vui.aspect_ratio_idc = static_cast<std::uint8_t>(*aspect_ratio_idc);
        if (*aspect_ratio_idc == 255) {
            const auto sar_width = reader.read_bits(16);
            const auto sar_height = reader.read_bits(16);
            if (!sar_width.has_value() || !sar_height.has_value()) {
                return false;
            }
            vui.sar_width = static_cast<std::uint16_t>(*sar_width);
            vui.sar_height = static_cast<std::uint16_t>(*sar_height);
        }
    }

    const auto overscan_info_present_flag = reader.read_bit();
    if (!overscan_info_present_flag.has_value()) {
        return false;
    }
    if (*overscan_info_present_flag && !reader.read_bit().has_value()) {
        return false;
    }

    const auto video_signal_type_present_flag = reader.read_bit();
    if (!video_signal_type_present_flag.has_value()) {
        return false;
    }
    vui.video_signal_type_present_flag = *video_signal_type_present_flag;
    if (*video_signal_type_present_flag) {
        const auto video_format = reader.read_bits(3);
        const auto video_full_range_flag = reader.read_bit();
        if (!video_format.has_value() || !video_full_range_flag.has_value()) {
            return false;
        }
        vui.video_format = static_cast<std::uint8_t>(*video_format);
        vui.video_full_range_flag = *video_full_range_flag;
        const auto colour_description_present_flag = reader.read_bit();
        if (!colour_description_present_flag.has_value()) {
            return false;
        }
        vui.colour_description_present_flag = *colour_description_present_flag;
        if (*colour_description_present_flag) {
            const auto colour_primaries = reader.read_bits(8);
            const auto transfer_characteristics = reader.read_bits(8);
            const auto matrix_coefficients = reader.read_bits(8);
            if (!colour_primaries.has_value() || !transfer_characteristics.has_value() ||
                !matrix_coefficients.has_value()) {
                return false;
            }
            vui.colour_primaries = static_cast<std::uint8_t>(*colour_primaries);
            vui.transfer_characteristics = static_cast<std::uint8_t>(*transfer_characteristics);
            vui.matrix_coefficients = static_cast<std::uint8_t>(*matrix_coefficients);
        }
    }

    const auto chroma_loc_info_present_flag = reader.read_bit();
    if (!chroma_loc_info_present_flag.has_value()) {
        return false;
    }
    if (*chroma_loc_info_present_flag && (!reader.read_ue().has_value() || !reader.read_ue().has_value())) {
        return false;
    }

    const auto timing_info_present_flag = reader.read_bit();
    if (!timing_info_present_flag.has_value()) {
        return false;
    }
    vui.timing_info_present_flag = *timing_info_present_flag;
    if (*timing_info_present_flag) {
        const auto num_units_in_tick = reader.read_bits(32);
        const auto time_scale = reader.read_bits(32);
        const auto fixed_frame_rate_flag = reader.read_bit();
        if (!num_units_in_tick.has_value() || !time_scale.has_value() ||
            !fixed_frame_rate_flag.has_value()) {
            return false;
        }
        vui.num_units_in_tick = static_cast<std::uint32_t>(*num_units_in_tick);
        vui.time_scale = static_cast<std::uint32_t>(*time_scale);
        vui.fixed_frame_rate_flag = *fixed_frame_rate_flag;
    }

    const auto nal_hrd_parameters_present_flag = reader.read_bit();
    if (!nal_hrd_parameters_present_flag.has_value()) {
        return false;
    }
    vui.nal_hrd_parameters_present_flag = *nal_hrd_parameters_present_flag;
    if (*nal_hrd_parameters_present_flag && !skip_hrd_parameters(reader)) {
        return false;
    }

    const auto vcl_hrd_parameters_present_flag = reader.read_bit();
    if (!vcl_hrd_parameters_present_flag.has_value()) {
        return false;
    }
    vui.vcl_hrd_parameters_present_flag = *vcl_hrd_parameters_present_flag;
    if (*vcl_hrd_parameters_present_flag && !skip_hrd_parameters(reader)) {
        return false;
    }

    if ((*nal_hrd_parameters_present_flag || *vcl_hrd_parameters_present_flag) && !reader.read_bit().has_value()) {
        return false;
    }

    return reader.read_bit().has_value() && reader.read_bit().has_value();
}

[[nodiscard]] std::uint32_t crop_unit_x(std::uint32_t chroma_format_idc) {
    if (chroma_format_idc == 0 || chroma_format_idc == 3) {
        return 1;
    }
    return 2;
}

[[nodiscard]] std::uint32_t crop_unit_y(std::uint32_t chroma_format_idc, bool frame_mbs_only_flag) {
    const std::uint32_t frame_multiplier = frame_mbs_only_flag ? 1 : 2;
    if (chroma_format_idc == 0) {
        return frame_multiplier;
    }
    if (chroma_format_idc == 1) {
        return 2 * frame_multiplier;
    }
    return frame_multiplier;
}

} // namespace

H264SpsParseResult parse_h264_sps(std::span<const std::uint8_t> nal_payload) {
    if (nal_payload.size() < 4) {
        return {Status::parse_error("H.264 SPS NAL payload is too small"), std::nullopt};
    }

    const auto rbsp = nal_payload_to_rbsp(nal_payload);
    BitReader reader(std::span<const std::uint8_t>(rbsp).subspan(1));

    H264SpsInfo info{};
    const auto profile_idc = reader.read_bits(8);
    const auto constraint_flags = reader.read_bits(8);
    const auto level_idc = reader.read_bits(8);
    const auto seq_parameter_set_id = reader.read_ue();
    if (!profile_idc.has_value() || !constraint_flags.has_value() || !level_idc.has_value() ||
        !seq_parameter_set_id.has_value()) {
        return {Status::parse_error("failed to read H.264 SPS header"), std::nullopt};
    }

    info.profile_idc = static_cast<std::uint8_t>(*profile_idc);
    info.constraint_flags = static_cast<std::uint8_t>(*constraint_flags);
    info.level_idc = static_cast<std::uint8_t>(*level_idc);
    info.seq_parameter_set_id = *seq_parameter_set_id;

    if (is_high_profile(info.profile_idc)) {
        const auto chroma_format_idc = reader.read_ue();
        if (!chroma_format_idc.has_value()) {
            return {Status::parse_error("failed to read chroma_format_idc"), std::nullopt};
        }
        info.chroma_format_idc = *chroma_format_idc;
        if (info.chroma_format_idc == 3 && !reader.read_bit().has_value()) {
            return {Status::parse_error("failed to read separate_colour_plane_flag"), std::nullopt};
        }

        const auto bit_depth_luma_minus8 = reader.read_ue();
        const auto bit_depth_chroma_minus8 = reader.read_ue();
        if (!bit_depth_luma_minus8.has_value() || !bit_depth_chroma_minus8.has_value()) {
            return {Status::parse_error("failed to read bit depth"), std::nullopt};
        }
        info.bit_depth_luma = static_cast<std::uint8_t>(*bit_depth_luma_minus8 + 8);
        info.bit_depth_chroma = static_cast<std::uint8_t>(*bit_depth_chroma_minus8 + 8);

        if (!reader.read_bit().has_value()) {
            return {Status::parse_error("failed to read qpprime_y_zero_transform_bypass_flag"), std::nullopt};
        }
        const auto seq_scaling_matrix_present_flag = reader.read_bit();
        if (!seq_scaling_matrix_present_flag.has_value()) {
            return {Status::parse_error("failed to read seq_scaling_matrix_present_flag"), std::nullopt};
        }
        if (*seq_scaling_matrix_present_flag) {
            const std::size_t scaling_list_count = info.chroma_format_idc == 3 ? 12 : 8;
            for (std::size_t i = 0; i < scaling_list_count; ++i) {
                const auto seq_scaling_list_present_flag = reader.read_bit();
                if (!seq_scaling_list_present_flag.has_value()) {
                    return {Status::parse_error("failed to read scaling list flag"), std::nullopt};
                }
                if (*seq_scaling_list_present_flag && !skip_scaling_list(reader, i < 6 ? 16 : 64)) {
                    return {Status::parse_error("failed to skip scaling list"), std::nullopt};
                }
            }
        }
    }

    const auto log2_max_frame_num_minus4 = reader.read_ue();
    if (!log2_max_frame_num_minus4.has_value()) {
        return {Status::parse_error("failed to read log2_max_frame_num_minus4"), std::nullopt};
    }
    info.log2_max_frame_num_minus4 = *log2_max_frame_num_minus4;
    const auto pic_order_cnt_type = reader.read_ue();
    if (!pic_order_cnt_type.has_value()) {
        return {Status::parse_error("failed to read pic_order_cnt_type"), std::nullopt};
    }
    info.pic_order_cnt_type = *pic_order_cnt_type;
    if (info.pic_order_cnt_type == 0) {
        const auto log2_max_pic_order_cnt_lsb_minus4 = reader.read_ue();
        if (!log2_max_pic_order_cnt_lsb_minus4.has_value()) {
            return {Status::parse_error("failed to read log2_max_pic_order_cnt_lsb_minus4"), std::nullopt};
        }
        info.log2_max_pic_order_cnt_lsb_minus4 = *log2_max_pic_order_cnt_lsb_minus4;
    } else if (info.pic_order_cnt_type == 1) {
        const auto delta_pic_order_always_zero_flag = reader.read_bit();
        if (!delta_pic_order_always_zero_flag.has_value() || !reader.read_se().has_value() ||
            !reader.read_se().has_value()) {
            return {Status::parse_error("failed to read POC type 1 fields"), std::nullopt};
        }
        info.delta_pic_order_always_zero_flag = *delta_pic_order_always_zero_flag;
        const auto num_ref_frames_in_pic_order_cnt_cycle = reader.read_ue();
        if (!num_ref_frames_in_pic_order_cnt_cycle.has_value()) {
            return {Status::parse_error("failed to read POC cycle count"), std::nullopt};
        }
        for (std::uint32_t i = 0; i < *num_ref_frames_in_pic_order_cnt_cycle; ++i) {
            if (!reader.read_se().has_value()) {
                return {Status::parse_error("failed to read offset_for_ref_frame"), std::nullopt};
            }
        }
    } else if (info.pic_order_cnt_type > 2) {
        return {Status::parse_error("invalid pic_order_cnt_type"), std::nullopt};
    }

    if (!reader.read_ue().has_value() || !reader.read_bit().has_value()) {
        return {Status::parse_error("failed to read reference frame fields"), std::nullopt};
    }

    const auto pic_width_in_mbs_minus1 = reader.read_ue();
    const auto pic_height_in_map_units_minus1 = reader.read_ue();
    const auto frame_mbs_only_flag = reader.read_bit();
    if (!pic_width_in_mbs_minus1.has_value() || !pic_height_in_map_units_minus1.has_value() ||
        !frame_mbs_only_flag.has_value()) {
        return {Status::parse_error("failed to read picture dimensions"), std::nullopt};
    }

    info.pic_width_in_mbs_minus1 = *pic_width_in_mbs_minus1;
    info.pic_height_in_map_units_minus1 = *pic_height_in_map_units_minus1;
    info.frame_mbs_only_flag = *frame_mbs_only_flag;

    if (!info.frame_mbs_only_flag && !reader.read_bit().has_value()) {
        return {Status::parse_error("failed to read mb_adaptive_frame_field_flag"), std::nullopt};
    }
    if (!reader.read_bit().has_value()) {
        return {Status::parse_error("failed to read direct_8x8_inference_flag"), std::nullopt};
    }

    const auto frame_cropping_flag = reader.read_bit();
    if (!frame_cropping_flag.has_value()) {
        return {Status::parse_error("failed to read frame_cropping_flag"), std::nullopt};
    }
    info.frame_cropping_flag = *frame_cropping_flag;
    if (info.frame_cropping_flag) {
        const auto left = reader.read_ue();
        const auto right = reader.read_ue();
        const auto top = reader.read_ue();
        const auto bottom = reader.read_ue();
        if (!left.has_value() || !right.has_value() || !top.has_value() || !bottom.has_value()) {
            return {Status::parse_error("failed to read frame crop offsets"), std::nullopt};
        }
        info.frame_crop_left_offset = *left;
        info.frame_crop_right_offset = *right;
        info.frame_crop_top_offset = *top;
        info.frame_crop_bottom_offset = *bottom;
    }

    const auto vui_parameters_present_flag = reader.read_bit();
    if (!vui_parameters_present_flag.has_value()) {
        return {Status::parse_error("failed to read vui_parameters_present_flag"), std::nullopt};
    }
    if (*vui_parameters_present_flag) {
        H264VuiInfo vui{};
        if (!parse_vui_parameters(reader, vui)) {
            return {Status::parse_error("failed to parse VUI parameters"), std::nullopt};
        }
        info.vui = vui;
    }

    const std::uint32_t coded_width = (info.pic_width_in_mbs_minus1 + 1) * 16;
    const std::uint32_t coded_height = (2 - static_cast<std::uint32_t>(info.frame_mbs_only_flag)) *
                                       (info.pic_height_in_map_units_minus1 + 1) * 16;
    const std::uint32_t crop_x = crop_unit_x(info.chroma_format_idc);
    const std::uint32_t crop_y = crop_unit_y(info.chroma_format_idc, info.frame_mbs_only_flag);
    const std::uint32_t crop_width = (info.frame_crop_left_offset + info.frame_crop_right_offset) * crop_x;
    const std::uint32_t crop_height = (info.frame_crop_top_offset + info.frame_crop_bottom_offset) * crop_y;

    if (crop_width >= coded_width || crop_height >= coded_height) {
        return {Status::parse_error("invalid H.264 SPS cropping offsets"), std::nullopt};
    }

    info.width = coded_width - crop_width;
    info.height = coded_height - crop_height;

    return {Status::ok(), info};
}

} // namespace streamview::bitstream
