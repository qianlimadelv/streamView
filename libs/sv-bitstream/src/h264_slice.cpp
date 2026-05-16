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
    return parse_h264_slice_header(
        nal_payload,
        H264SliceHeaderContext{
            .log2_max_frame_num_minus4 = log2_max_frame_num_minus4,
            .pic_order_cnt_type = 2,
        });
}

H264SliceHeaderParseResult parse_h264_slice_header(
    std::span<const std::uint8_t> nal_payload,
    const H264SliceHeaderContext& context) {
    if (nal_payload.size() < 2) {
        return {Status::parse_error("H.264 slice NAL payload is too small"), std::nullopt};
    }

    if (context.log2_max_frame_num_minus4 > 12) {
        return {Status::parse_error("invalid log2_max_frame_num_minus4"), std::nullopt};
    }
    if (context.log2_max_pic_order_cnt_lsb_minus4 > 12) {
        return {Status::parse_error("invalid log2_max_pic_order_cnt_lsb_minus4"), std::nullopt};
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

    const auto frame_num = reader.read_bits(context.log2_max_frame_num_minus4 + 4);
    if (!frame_num.has_value()) {
        return {Status::parse_error("failed to read H.264 frame_num"), std::nullopt};
    }

    info.first_mb_in_slice = *first_mb_in_slice;
    info.slice_type_raw = *slice_type_raw;
    info.slice_kind = *slice_kind;
    info.slice_type_all_slices = *slice_type_raw >= 5;
    info.pic_parameter_set_id = *pic_parameter_set_id;
    info.frame_num = *frame_num;

    if (!context.frame_mbs_only_flag) {
        const auto field_pic_flag = reader.read_bit();
        if (!field_pic_flag.has_value()) {
            return {Status::parse_error("failed to read H.264 field_pic_flag"), std::nullopt};
        }
        info.field_pic_flag_present = true;
        info.field_pic_flag = *field_pic_flag;
        if (info.field_pic_flag) {
            const auto bottom_field_flag = reader.read_bit();
            if (!bottom_field_flag.has_value()) {
                return {Status::parse_error("failed to read H.264 bottom_field_flag"), std::nullopt};
            }
            info.bottom_field_flag_present = true;
            info.bottom_field_flag = *bottom_field_flag;
        }
    }

    if (context.is_idr) {
        const auto idr_pic_id = reader.read_ue();
        if (!idr_pic_id.has_value()) {
            return {Status::ok(), info};
        }
        info.idr_pic_id_present = true;
        info.idr_pic_id = *idr_pic_id;
    }

    if (context.pic_order_cnt_type == 0) {
        const auto pic_order_cnt_lsb = reader.read_bits(context.log2_max_pic_order_cnt_lsb_minus4 + 4);
        if (!pic_order_cnt_lsb.has_value()) {
            return {Status::ok(), info};
        }
        info.pic_order_cnt_lsb_present = true;
        info.pic_order_cnt_lsb = *pic_order_cnt_lsb;
        if (context.bottom_field_pic_order_in_frame_present_flag && !info.field_pic_flag) {
            const auto delta_pic_order_cnt_bottom = reader.read_se();
            if (!delta_pic_order_cnt_bottom.has_value()) {
                return {Status::ok(), info};
            }
            info.delta_pic_order_cnt_bottom_present = true;
            info.delta_pic_order_cnt_bottom = *delta_pic_order_cnt_bottom;
        }
    }

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
