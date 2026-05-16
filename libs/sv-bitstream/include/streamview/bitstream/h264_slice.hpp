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
    bool field_pic_flag_present{};
    bool field_pic_flag{};
    bool bottom_field_flag_present{};
    bool bottom_field_flag{};
    bool idr_pic_id_present{};
    std::uint32_t idr_pic_id{};
    bool pic_order_cnt_lsb_present{};
    std::uint32_t pic_order_cnt_lsb{};
    bool delta_pic_order_cnt_bottom_present{};
    std::int32_t delta_pic_order_cnt_bottom{};
};

struct H264SliceHeaderContext {
    std::uint32_t log2_max_frame_num_minus4{};
    bool frame_mbs_only_flag{true};
    std::uint32_t pic_order_cnt_type{};
    std::uint32_t log2_max_pic_order_cnt_lsb_minus4{};
    bool bottom_field_pic_order_in_frame_present_flag{};
    bool is_idr{};
};

struct H264SliceHeaderParseResult {
    streamview::Status status;
    std::optional<H264SliceHeaderInfo> info;
};

[[nodiscard]] H264SliceHeaderParseResult parse_h264_slice_header(
    std::span<const std::uint8_t> nal_payload,
    std::uint32_t log2_max_frame_num_minus4);

[[nodiscard]] H264SliceHeaderParseResult parse_h264_slice_header(
    std::span<const std::uint8_t> nal_payload,
    const H264SliceHeaderContext& context);

[[nodiscard]] std::string_view h264_slice_kind_name(H264SliceKind kind);

} // namespace streamview::bitstream
