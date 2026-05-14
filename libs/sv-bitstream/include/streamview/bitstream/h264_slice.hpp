#pragma once

#include "streamview/core/status.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace streamview::bitstream {

enum class H264SliceKind {
    P,
    B,
    I,
    SP,
    SI,
};

struct H264SliceHeaderInfo {
    std::uint32_t first_mb_in_slice{};
    std::uint32_t slice_type_raw{};
    H264SliceKind slice_kind{H264SliceKind::P};
    bool slice_type_all_slices{};
    std::uint32_t pic_parameter_set_id{};
    std::uint32_t frame_num{};
};

struct H264SliceHeaderParseResult {
    streamview::Status status;
    std::optional<H264SliceHeaderInfo> info;
};

[[nodiscard]] H264SliceHeaderParseResult parse_h264_slice_header(
    std::span<const std::uint8_t> nal_payload,
    std::uint32_t log2_max_frame_num_minus4);

[[nodiscard]] std::string_view h264_slice_kind_name(H264SliceKind kind);

} // namespace streamview::bitstream
