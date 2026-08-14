#pragma once

#include "streamview/core/status.hpp"

#include <cstdint>
#include <optional>
#include <span>

namespace streamview::bitstream {

struct H265PpsInfo {
    std::uint32_t pic_parameter_set_id{};
    std::uint32_t seq_parameter_set_id{};
    bool dependent_slice_segments_enabled_flag{};
    bool output_flag_present_flag{};
    std::uint8_t num_extra_slice_header_bits{};

    bool operator==(const H265PpsInfo&) const = default;
};

struct H265PpsParseResult {
    streamview::Status status;
    std::optional<H265PpsInfo> info;
};

[[nodiscard]] H265PpsParseResult parse_h265_pps(std::span<const std::uint8_t> nal_payload);

} // namespace streamview::bitstream
