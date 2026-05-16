#pragma once

#include "streamview/bitstream/h265_nal.hpp"
#include "streamview/core/status.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace streamview::bitstream {

enum class H265SliceKind {
    B,
    P,
    I,
};

struct H265SliceHeaderInfo {
    bool first_slice_segment_in_pic_flag{};
    bool no_output_of_prior_pics_flag{};
    bool no_output_of_prior_pics_flag_present{};
    std::uint32_t slice_pic_parameter_set_id{};
    bool slice_type_present{};
    std::uint32_t slice_type_raw{};
    H265SliceKind slice_kind{H265SliceKind::B};
    bool dependent_slice_segment_flag_present{};
    bool dependent_slice_segment_flag{};
    bool pic_output_flag_present{};
    bool pic_output_flag{};
    bool pic_order_cnt_lsb_present{};
    std::uint32_t pic_order_cnt_lsb{};
};

struct H265SliceHeaderContext {
    std::optional<std::uint8_t> num_extra_slice_header_bits;
    std::optional<std::uint32_t> log2_max_pic_order_cnt_lsb_minus4;
    bool dependent_slice_segments_enabled_flag{};
    bool output_flag_present_flag{};
    bool is_irap{};
};

struct H265SliceHeaderParseResult {
    streamview::Status status;
    std::optional<H265SliceHeaderInfo> info;
};

[[nodiscard]] H265SliceHeaderParseResult parse_h265_slice_header(
    std::span<const std::uint8_t> nal_payload,
    H265NalType nal_unit_type,
    std::optional<std::uint8_t> num_extra_slice_header_bits);

[[nodiscard]] H265SliceHeaderParseResult parse_h265_slice_header(
    std::span<const std::uint8_t> nal_payload,
    H265NalType nal_unit_type,
    const H265SliceHeaderContext& context);

[[nodiscard]] std::string_view h265_slice_kind_name(H265SliceKind kind);

} // namespace streamview::bitstream
