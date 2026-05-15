#pragma once

#include "streamview/core/status.hpp"

#include <cstdint>
#include <optional>
#include <span>

namespace streamview::bitstream {

struct H265VpsInfo {
    std::uint8_t profile_idc{};
    bool tier_flag{};
    std::uint8_t level_idc{};
    std::uint32_t video_parameter_set_id{};
    bool base_layer_internal_flag{};
    bool base_layer_available_flag{};
    std::uint32_t max_layers_minus1{};
    std::uint32_t max_sub_layers_minus1{};
    bool temporal_id_nesting_flag{};
    std::uint32_t max_dec_pic_buffering_minus1{};
    std::uint32_t max_num_reorder_pics{};
    std::uint32_t max_latency_increase_plus1{};
    std::uint32_t max_layer_id{};
    std::uint32_t num_layer_sets_minus1{};
};

struct H265VpsParseResult {
    streamview::Status status;
    std::optional<H265VpsInfo> info;
};

[[nodiscard]] H265VpsParseResult parse_h265_vps(std::span<const std::uint8_t> nal_payload);

} // namespace streamview::bitstream
