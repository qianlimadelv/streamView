#include "streamview/bitstream/h265_sps.hpp"

#include "streamview/bitstream/bit_reader.hpp"
#include "streamview/bitstream/rbsp.hpp"

#include <algorithm>
#include <array>
#include <vector>

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

[[nodiscard]] bool skip_scaling_list_data(BitReader& reader) {
    for (int sizeId = 0; sizeId < 4; ++sizeId) {
        for (int matrixId = 0; matrixId < 6; matrixId += (sizeId == 3 ? 3 : 1)) {
            const auto pred_mode = reader.read_bit();
            if (!pred_mode.has_value()) {
                return false;
            }
            if (!*pred_mode) {
                if (!reader.read_ue().has_value()) { // scaling_list_pred_matrix_id_delta
                    return false;
                }
            } else {
                const int coef_num = std::min(64, 1 << (4 + (sizeId << 1)));
                if (sizeId > 1 && !reader.read_se().has_value()) { // scaling_list_dc_coef_minus8
                    return false;
                }
                for (int i = 0; i < coef_num; ++i) {
                    if (!reader.read_se().has_value()) { // scaling_list_delta_coef
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

// st_ref_pic_set (ITU-T H.265 7.3.7). Maintains NumDeltaPocs per RPS index so
// the inter-prediction branch can be skipped correctly.
[[nodiscard]] bool skip_st_ref_pic_set(BitReader& reader, std::uint32_t idx,
                                       std::vector<int>& num_delta_pocs) {
    bool inter_pred = false;
    if (idx != 0) {
        const auto flag = reader.read_bit();
        if (!flag.has_value()) {
            return false;
        }
        inter_pred = *flag;
    }
    if (inter_pred) {
        // In the SPS list stRpsIdx < num_short_term_ref_pic_sets, so RefRpsIdx = idx - 1.
        if (!reader.read_bit().has_value() || !reader.read_ue().has_value()) { // delta_rps_sign, abs_delta_rps_minus1
            return false;
        }
        const int ref = static_cast<int>(idx) - 1;
        const int ref_num = (ref >= 0 && ref < static_cast<int>(num_delta_pocs.size())) ? num_delta_pocs[ref] : 0;
        int count = 0;
        for (int j = 0; j <= ref_num; ++j) {
            const auto used = reader.read_bit();
            if (!used.has_value()) {
                return false;
            }
            bool use_delta = true;
            if (!*used) {
                const auto ud = reader.read_bit();
                if (!ud.has_value()) {
                    return false;
                }
                use_delta = *ud;
            }
            if (*used || use_delta) {
                ++count;
            }
        }
        num_delta_pocs[idx] = count;
    } else {
        const auto neg = reader.read_ue();
        const auto pos = reader.read_ue();
        if (!neg.has_value() || !pos.has_value()) {
            return false;
        }
        num_delta_pocs[idx] = static_cast<int>(*neg + *pos);
        for (std::uint32_t i = 0; i < *neg; ++i) {
            if (!reader.read_ue().has_value() || !reader.read_bit().has_value()) {
                return false;
            }
        }
        for (std::uint32_t i = 0; i < *pos; ++i) {
            if (!reader.read_ue().has_value() || !reader.read_bit().has_value()) {
                return false;
            }
        }
    }
    return true;
}

// VUI parsed up to timing info (we don't need HRD, which follows).
[[nodiscard]] bool parse_h265_vui(BitReader& reader, H265VuiInfo& vui) {
    const auto aspect = reader.read_bit();
    if (!aspect.has_value()) {
        return false;
    }
    vui.aspect_ratio_info_present_flag = *aspect;
    if (*aspect) {
        const auto idc = reader.read_bits(8);
        if (!idc.has_value()) {
            return false;
        }
        vui.aspect_ratio_idc = static_cast<std::uint8_t>(*idc);
        if (*idc == 255) {
            const auto w = reader.read_bits(16);
            const auto h = reader.read_bits(16);
            if (!w.has_value() || !h.has_value()) {
                return false;
            }
            vui.sar_width = static_cast<std::uint16_t>(*w);
            vui.sar_height = static_cast<std::uint16_t>(*h);
        }
    }
    const auto overscan = reader.read_bit();
    if (!overscan.has_value()) {
        return false;
    }
    if (*overscan && !reader.read_bit().has_value()) {
        return false;
    }
    const auto video_signal = reader.read_bit();
    if (!video_signal.has_value()) {
        return false;
    }
    vui.video_signal_type_present_flag = *video_signal;
    if (*video_signal) {
        const auto vf = reader.read_bits(3);
        const auto fr = reader.read_bit();
        if (!vf.has_value() || !fr.has_value()) {
            return false;
        }
        vui.video_format = static_cast<std::uint8_t>(*vf);
        vui.video_full_range_flag = *fr;
        const auto cd = reader.read_bit();
        if (!cd.has_value()) {
            return false;
        }
        vui.colour_description_present_flag = *cd;
        if (*cd) {
            const auto cp = reader.read_bits(8);
            const auto tc = reader.read_bits(8);
            const auto mc = reader.read_bits(8);
            if (!cp.has_value() || !tc.has_value() || !mc.has_value()) {
                return false;
            }
            vui.colour_primaries = static_cast<std::uint8_t>(*cp);
            vui.transfer_characteristics = static_cast<std::uint8_t>(*tc);
            vui.matrix_coefficients = static_cast<std::uint8_t>(*mc);
        }
    }
    const auto chroma_loc = reader.read_bit();
    if (!chroma_loc.has_value()) {
        return false;
    }
    if (*chroma_loc && (!reader.read_ue().has_value() || !reader.read_ue().has_value())) {
        return false;
    }
    // neutral_chroma_indication_flag, field_seq_flag, frame_field_info_present_flag
    if (!reader.read_bit().has_value() || !reader.read_bit().has_value() || !reader.read_bit().has_value()) {
        return false;
    }
    const auto ddw = reader.read_bit();
    if (!ddw.has_value()) {
        return false;
    }
    if (*ddw && (!reader.read_ue().has_value() || !reader.read_ue().has_value() ||
                 !reader.read_ue().has_value() || !reader.read_ue().has_value())) {
        return false;
    }
    const auto timing = reader.read_bit();
    if (!timing.has_value()) {
        return false;
    }
    vui.vui_timing_info_present_flag = *timing;
    if (*timing) {
        const auto num_units = reader.read_bits(32);
        const auto time_scale = reader.read_bits(32);
        if (!num_units.has_value() || !time_scale.has_value()) {
            return false;
        }
        vui.vui_num_units_in_tick = static_cast<std::uint32_t>(*num_units);
        vui.vui_time_scale = static_cast<std::uint32_t>(*time_scale);
    }
    return true;
}

// Best-effort parse from sps_sub_layer_ordering_info up to VUI. Failure leaves
// info.vui empty but does not invalidate the already-extracted SPS fields.
void parse_h265_sps_tail(BitReader& reader, H265SpsInfo& info) {
    const auto ordering = reader.read_bit();
    if (!ordering.has_value()) {
        return;
    }
    const std::uint32_t start = *ordering ? 0 : info.max_sub_layers_minus1;
    for (std::uint32_t i = start; i <= info.max_sub_layers_minus1; ++i) {
        if (!reader.read_ue().has_value() || !reader.read_ue().has_value() || !reader.read_ue().has_value()) {
            return;
        }
    }
    for (int i = 0; i < 6; ++i) { // 4 log2 cb/tb sizes + 2 max_transform_hierarchy_depth
        if (!reader.read_ue().has_value()) {
            return;
        }
    }
    const auto scaling = reader.read_bit();
    if (!scaling.has_value()) {
        return;
    }
    if (*scaling) {
        const auto present = reader.read_bit();
        if (!present.has_value()) {
            return;
        }
        if (*present && !skip_scaling_list_data(reader)) {
            return;
        }
    }
    if (!reader.read_bit().has_value() || !reader.read_bit().has_value()) { // amp, sao
        return;
    }
    const auto pcm = reader.read_bit();
    if (!pcm.has_value()) {
        return;
    }
    if (*pcm) {
        if (!skip_bits(reader, 8) || !reader.read_ue().has_value() || !reader.read_ue().has_value() ||
            !reader.read_bit().has_value()) {
            return;
        }
    }
    const auto num_rps = reader.read_ue();
    if (!num_rps.has_value() || *num_rps > 64) {
        return;
    }
    std::vector<int> num_delta_pocs(*num_rps + 1, 0);
    for (std::uint32_t i = 0; i < *num_rps; ++i) {
        if (!skip_st_ref_pic_set(reader, i, num_delta_pocs)) {
            return;
        }
    }
    const auto ltp = reader.read_bit();
    if (!ltp.has_value()) {
        return;
    }
    if (*ltp) {
        const auto num_lt = reader.read_ue();
        if (!num_lt.has_value()) {
            return;
        }
        const std::size_t poc_len = info.log2_max_pic_order_cnt_lsb_minus4 + 4;
        for (std::uint32_t i = 0; i < *num_lt; ++i) {
            if (!skip_bits(reader, poc_len) || !reader.read_bit().has_value()) {
                return;
            }
        }
    }
    if (!reader.read_bit().has_value() || !reader.read_bit().has_value()) { // temporal_mvp, strong_intra_smoothing
        return;
    }
    const auto vui_present = reader.read_bit();
    if (!vui_present.has_value()) {
        return;
    }
    if (*vui_present) {
        H265VuiInfo vui{};
        if (parse_h265_vui(reader, vui)) {
            info.vui = vui;
        }
    }
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

    // Best-effort: reach and parse VUI. Never fails the SPS (fields above stand).
    parse_h265_sps_tail(reader, info);

    return {Status::ok(), info};
}

} // namespace streamview::bitstream
