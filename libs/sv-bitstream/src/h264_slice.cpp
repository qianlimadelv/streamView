#include "streamview/bitstream/h264_slice.hpp"

#include "streamview/bitstream/bit_reader.hpp"
#include "streamview/bitstream/rbsp.hpp"

namespace streamview::bitstream {
namespace {

[[nodiscard]] std::optional<H264SliceKind> to_slice_kind(std::uint32_t slice_type_raw) {
    const std::uint32_t normalized = slice_type_raw % 5;
    switch (normalized) {
    case 0:
        return H264SliceKind::P;
    case 1:
        return H264SliceKind::B;
    case 2:
        return H264SliceKind::I;
    case 3:
        return H264SliceKind::SP;
    case 4:
        return H264SliceKind::SI;
    default:
        return std::nullopt;
    }
}

} // namespace

H264SliceHeaderParseResult parse_h264_slice_header(
    std::span<const std::uint8_t> nal_payload,
    std::uint32_t log2_max_frame_num_minus4) {
    if (nal_payload.size() < 2) {
        return {Status::parse_error("H.264 slice NAL payload is too small"), std::nullopt};
    }

    if (log2_max_frame_num_minus4 > 12) {
        return {Status::parse_error("invalid log2_max_frame_num_minus4"), std::nullopt};
    }

    const auto rbsp = nal_payload_to_rbsp(nal_payload);
    BitReader reader(std::span<const std::uint8_t>(rbsp).subspan(1));

    H264SliceHeaderInfo info{};
    const auto first_mb_in_slice = reader.read_ue();
    const auto slice_type_raw = reader.read_ue();
    const auto pic_parameter_set_id = reader.read_ue();
    if (!first_mb_in_slice.has_value() || !slice_type_raw.has_value() || !pic_parameter_set_id.has_value()) {
        return {Status::parse_error("failed to read H.264 slice header prefix"), std::nullopt};
    }

    const auto slice_kind = to_slice_kind(*slice_type_raw);
    if (!slice_kind.has_value()) {
        return {Status::parse_error("invalid H.264 slice_type"), std::nullopt};
    }

    const auto frame_num = reader.read_bits(log2_max_frame_num_minus4 + 4);
    if (!frame_num.has_value()) {
        return {Status::parse_error("failed to read H.264 frame_num"), std::nullopt};
    }

    info.first_mb_in_slice = *first_mb_in_slice;
    info.slice_type_raw = *slice_type_raw;
    info.slice_kind = *slice_kind;
    info.slice_type_all_slices = *slice_type_raw >= 5;
    info.pic_parameter_set_id = *pic_parameter_set_id;
    info.frame_num = *frame_num;

    return {Status::ok(), info};
}

std::string_view h264_slice_kind_name(H264SliceKind kind) {
    switch (kind) {
    case H264SliceKind::P:
        return "P";
    case H264SliceKind::B:
        return "B";
    case H264SliceKind::I:
        return "I";
    case H264SliceKind::SP:
        return "SP";
    case H264SliceKind::SI:
        return "SI";
    }
    return "unknown";
}

} // namespace streamview::bitstream
