#pragma once

#include "streamview/core/status.hpp"

#include <cstdint>
#include <optional>
#include <span>

namespace streamview::bitstream {

// VUI parameters (ITU-T H.264 E.1.1) — the fields StreamEye-class tools surface.
struct H264VuiInfo {
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
    bool timing_info_present_flag{};
    std::uint32_t num_units_in_tick{};
    std::uint32_t time_scale{};
    bool fixed_frame_rate_flag{};
    bool nal_hrd_parameters_present_flag{};
    bool vcl_hrd_parameters_present_flag{};
};

struct H264SpsInfo {
    std::uint8_t profile_idc{};
    std::uint8_t constraint_flags{};
    std::uint8_t level_idc{};
    std::uint32_t seq_parameter_set_id{};
    std::uint32_t chroma_format_idc{1};
    std::uint8_t bit_depth_luma{8};
    std::uint8_t bit_depth_chroma{8};
    std::uint32_t log2_max_frame_num_minus4{};
    std::uint32_t pic_order_cnt_type{};
    std::uint32_t log2_max_pic_order_cnt_lsb_minus4{};
    bool delta_pic_order_always_zero_flag{};
    std::uint32_t pic_width_in_mbs_minus1{};
    std::uint32_t pic_height_in_map_units_minus1{};
    bool frame_mbs_only_flag{};
    bool frame_cropping_flag{};
    std::uint32_t frame_crop_left_offset{};
    std::uint32_t frame_crop_right_offset{};
    std::uint32_t frame_crop_top_offset{};
    std::uint32_t frame_crop_bottom_offset{};
    std::uint32_t width{};
    std::uint32_t height{};
    std::optional<H264VuiInfo> vui;
};

struct H264SpsParseResult {
    streamview::Status status;
    std::optional<H264SpsInfo> info;
};

[[nodiscard]] H264SpsParseResult parse_h264_sps(std::span<const std::uint8_t> nal_payload);

} // namespace streamview::bitstream
