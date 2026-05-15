#pragma once

#include "streamview/bitstream/h265_nal.hpp"
#include "streamview/core/status.hpp"

#include <cstdint>
#include <optional>
#include <span>

namespace streamview::bitstream {

struct H265SliceHeaderInfo {
    bool first_slice_segment_in_pic_flag{};
    bool no_output_of_prior_pics_flag{};
    bool no_output_of_prior_pics_flag_present{};
    std::uint32_t slice_pic_parameter_set_id{};
};

struct H265SliceHeaderParseResult {
    streamview::Status status;
    std::optional<H265SliceHeaderInfo> info;
};

[[nodiscard]] H265SliceHeaderParseResult parse_h265_slice_header(
    std::span<const std::uint8_t> nal_payload,
    H265NalType nal_unit_type);

} // namespace streamview::bitstream
