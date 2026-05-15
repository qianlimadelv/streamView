#pragma once

#include "streamview/core/status.hpp"

#include <cstdint>
#include <optional>
#include <span>

namespace streamview::bitstream {

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
    std::uint32_t width{};
    std::uint32_t height{};
    bool conformance_window_flag{};
    std::uint32_t conf_win_left_offset{};
    std::uint32_t conf_win_right_offset{};
    std::uint32_t conf_win_top_offset{};
    std::uint32_t conf_win_bottom_offset{};
};

struct H265SpsParseResult {
    streamview::Status status;
    std::optional<H265SpsInfo> info;
};

[[nodiscard]] H265SpsParseResult parse_h265_sps(std::span<const std::uint8_t> nal_payload);

} // namespace streamview::bitstream
