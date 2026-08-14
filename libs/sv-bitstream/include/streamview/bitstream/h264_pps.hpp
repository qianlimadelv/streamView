#pragma once

#include "streamview/core/status.hpp"

#include <cstdint>
#include <optional>
#include <span>

namespace streamview::bitstream {

struct H264PpsInfo {
    std::uint32_t pic_parameter_set_id{};
    std::uint32_t seq_parameter_set_id{};
    bool entropy_coding_mode_flag{};
    bool bottom_field_pic_order_in_frame_present_flag{};
    std::uint32_t num_slice_groups_minus1{};

    bool operator==(const H264PpsInfo&) const = default;
};

struct H264PpsParseResult {
    streamview::Status status;
    std::optional<H264PpsInfo> info;
};

[[nodiscard]] H264PpsParseResult parse_h264_pps(std::span<const std::uint8_t> nal_payload);

} // namespace streamview::bitstream
