#pragma once

#include "streamview/core/status.hpp"

#include <cstdint>
#include <optional>
#include <span>

namespace streamview::bitstream {

struct H264SpsInfo {
    std::uint8_t profile_idc{};
    std::uint8_t constraint_flags{};
    std::uint8_t level_idc{};
    std::uint32_t seq_parameter_set_id{};
    std::uint32_t chroma_format_idc{1};
    std::uint8_t bit_depth_luma{8};
    std::uint8_t bit_depth_chroma{8};
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
};

struct H264SpsParseResult {
    streamview::Status status;
    std::optional<H264SpsInfo> info;
};

[[nodiscard]] H264SpsParseResult parse_h264_sps(std::span<const std::uint8_t> nal_payload);

} // namespace streamview::bitstream
