#pragma once

#include "streamview/core/status.hpp"

#include <cstdint>
#include <optional>
#include <span>

namespace streamview::bitstream {

// H.265 VUI parameters (ITU-T H.265 E.2.1), parsed up to timing info.
struct H265VuiInfo {
    bool aspect_ratio_info_present_flag{};
    std::uint8_t aspect_ratio_idc{};
    std::uint16_t sar_width{};
    std::uint16_t sar_height{};
    bool video_signal_type_present_flag{};
    std::uint8_t video_format{5};
    bool video_full_range_flag{};
    bool colour_description_present_flag{};
    std::uint8_t colour_primaries{2};
    std::uint8_t transfer_characteristics{2};
    std::uint8_t matrix_coefficients{2};
    bool vui_timing_info_present_flag{};
    std::uint32_t vui_num_units_in_tick{};
    std::uint32_t vui_time_scale{};
};

struct H265SpsInfo {
    std::uint8_t profile_idc{};
    bool tier_flag{};
    std::uint8_t level_idc{};
    std::uint32_t video_parameter_set_id{};
    std::uint32_t max_sub_layers_minus1{};
    std::uint32_t seq_parameter_set_id{};
    std::uint32_t chroma_format_idc{1};
    bool separate_colour_plane_flag{};
    std::uint8_t bit_depth_luma{8};
    std::uint8_t bit_depth_chroma{8};
    std::uint32_t log2_max_pic_order_cnt_lsb_minus4{};
    std::uint32_t width{};
    std::uint32_t height{};
    bool conformance_window_flag{};
    std::uint32_t conf_win_left_offset{};
    std::uint32_t conf_win_right_offset{};
    std::uint32_t conf_win_top_offset{};
    std::uint32_t conf_win_bottom_offset{};
    std::optional<H265VuiInfo> vui;
};

struct H265SpsParseResult {
    streamview::Status status;
    std::optional<H265SpsInfo> info;
};

[[nodiscard]] H265SpsParseResult parse_h265_sps(std::span<const std::uint8_t> nal_payload);

} // namespace streamview::bitstream
