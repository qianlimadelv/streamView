#include "streamview/analysis/stream_analysis.hpp"

#include <string>
#include <utility>

namespace streamview::analysis {
namespace {

void add_slice_to_summary(StreamSummary& summary, bitstream::H264SliceKind kind) {
    ++summary.slices.total;
    switch (kind) {
    case bitstream::H264SliceKind::I:
        ++summary.slices.i;
        break;
    case bitstream::H264SliceKind::P:
        ++summary.slices.p;
        break;
    case bitstream::H264SliceKind::B:
        ++summary.slices.b;
        break;
    case bitstream::H264SliceKind::SP:
        ++summary.slices.sp;
        break;
    case bitstream::H264SliceKind::SI:
        ++summary.slices.si;
        break;
    }
}

std::string h264_frame_type_name(bitstream::H264SliceKind kind) {
    return std::string(bitstream::h264_slice_kind_name(kind));
}

bool h264_slice_is_keyframe(bitstream::H264NalType nal_type, bitstream::H264SliceKind slice_kind) {
    return nal_type == bitstream::H264NalType::CodedSliceIdr || slice_kind == bitstream::H264SliceKind::I ||
           slice_kind == bitstream::H264SliceKind::SI;
}

void add_h264_frame(StreamAnalysis& analysis, const NalAnalysis& nal) {
    if (!nal.h264.has_value() || !nal.h264->slice.has_value()) {
        return;
    }

    FrameAnalysis frame{};
    frame.index = analysis.frames.size();
    frame.codec = "h264";
    frame.frame_type = h264_frame_type_name(nal.h264->slice->slice_kind);
    frame.is_keyframe = h264_slice_is_keyframe(nal.h264->header.nal_unit_type, nal.h264->slice->slice_kind);
    frame.nal_indices.push_back(nal.index);
    frame.size_bytes = nal.unit.start_code_size + nal.unit.payload_size;
    frame.first_payload_offset = nal.unit.payload_offset;

    ++analysis.summary.frame_count;
    if (frame.is_keyframe) {
        ++analysis.summary.keyframe_count;
    }
    analysis.frames.push_back(std::move(frame));
}

} // namespace

StreamAnalysis analyze_h264_annex_b(std::string input_path, std::span<const std::uint8_t> data) {
    StreamAnalysis analysis{};
    analysis.input_path = std::move(input_path);
    analysis.size_bytes = data.size();

    const auto units = bitstream::scan_annex_b(data);
    analysis.nals.reserve(units.size());

    for (std::size_t i = 0; i < units.size(); ++i) {
        const auto& unit = units[i];
        const auto payload = data.subspan(unit.payload_offset, unit.payload_size);
        NalAnalysis nal{};
        nal.index = i;
        nal.unit = unit;
        nal.h264 = H264NalAnalysis{};
        nal.h264->header = bitstream::parse_h264_nal_header(data[unit.payload_offset]);

        if (nal.h264->header.nal_unit_type == bitstream::H264NalType::Sps) {
            ++analysis.summary.sps_count;
            const auto sps = bitstream::parse_h264_sps(payload);
            if (sps.status.is_ok() && sps.info.has_value()) {
                nal.h264->sps = *sps.info;
                analysis.summary.active_sps = *sps.info;
            } else {
                nal.h264->sps_parse_error = sps.status.message();
            }
        } else if (nal.h264->header.nal_unit_type == bitstream::H264NalType::Pps) {
            ++analysis.summary.pps_count;
            const auto pps = bitstream::parse_h264_pps(payload);
            if (pps.status.is_ok() && pps.info.has_value()) {
                nal.h264->pps = *pps.info;
            } else {
                nal.h264->pps_parse_error = pps.status.message();
            }
        } else if (nal.h264->header.nal_unit_type == bitstream::H264NalType::CodedSliceNonIdr ||
                   nal.h264->header.nal_unit_type == bitstream::H264NalType::CodedSliceIdr) {
            if (analysis.summary.active_sps.has_value()) {
                const auto slice = bitstream::parse_h264_slice_header(
                    payload,
                    analysis.summary.active_sps->log2_max_frame_num_minus4);
                if (slice.status.is_ok() && slice.info.has_value()) {
                    nal.h264->slice = *slice.info;
                    add_slice_to_summary(analysis.summary, slice.info->slice_kind);
                    add_h264_frame(analysis, nal);
                } else {
                    nal.h264->slice_parse_error = slice.status.message();
                }
            } else {
                nal.h264->slice_parse_error = "missing active SPS";
            }
        }

        analysis.nals.push_back(std::move(nal));
    }

    build_gops(analysis);
    return analysis;
}

} // namespace streamview::analysis
