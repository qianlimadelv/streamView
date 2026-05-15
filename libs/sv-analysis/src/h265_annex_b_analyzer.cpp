#include "streamview/analysis/stream_analysis.hpp"

#include <map>
#include <string>
#include <utility>

namespace streamview::analysis {
namespace {

std::string h265_frame_type_name(bitstream::H265NalType type) {
    if (type == bitstream::H265NalType::IdrWRadl || type == bitstream::H265NalType::IdrNLp) {
        return "IDR";
    }
    if (type == bitstream::H265NalType::CraNut) {
        return "CRA";
    }
    return "VCL";
}

std::string h265_frame_type_name(const H265NalAnalysis& h265) {
    if (h265.slice.has_value() && h265.slice->slice_type_present) {
        return std::string(bitstream::h265_slice_kind_name(h265.slice->slice_kind));
    }
    return h265_frame_type_name(h265.header.nal_unit_type);
}

bool h265_nal_is_keyframe(bitstream::H265NalType type) {
    return type == bitstream::H265NalType::IdrWRadl || type == bitstream::H265NalType::IdrNLp ||
           type == bitstream::H265NalType::CraNut;
}

void add_h265_frame(StreamAnalysis& analysis, const NalAnalysis& nal) {
    if (!nal.h265.has_value() || !bitstream::h265_nal_type_is_vcl(nal.h265->header.nal_unit_type)) {
        return;
    }

    FrameAnalysis frame{};
    frame.index = analysis.frames.size();
    frame.codec = "h265";
    frame.frame_type = h265_frame_type_name(*nal.h265);
    frame.is_keyframe = h265_nal_is_keyframe(nal.h265->header.nal_unit_type);
    frame.nal_indices.push_back(nal.index);
    frame.size_bytes = nal.unit.start_code_size + nal.unit.payload_size;
    frame.first_payload_offset = nal.unit.payload_offset;

    ++analysis.summary.frame_count;
    if (frame.is_keyframe) {
        ++analysis.summary.keyframe_count;
    }
    analysis.frames.push_back(std::move(frame));
}

void add_h265_slice_to_summary(StreamSummary& summary, const bitstream::H265SliceHeaderInfo& slice) {
    ++summary.slices.total;
    if (!slice.slice_type_present) {
        return;
    }

    switch (slice.slice_kind) {
    case bitstream::H265SliceKind::I:
        ++summary.slices.i;
        break;
    case bitstream::H265SliceKind::P:
        ++summary.slices.p;
        break;
    case bitstream::H265SliceKind::B:
        ++summary.slices.b;
        break;
    }
}

void add_parse_error(StreamSummary& summary, std::size_t ParseErrorStats::*field) {
    ++summary.parse_errors.total;
    ++(summary.parse_errors.*field);
}

} // namespace

StreamAnalysis analyze_h265_annex_b(std::string input_path, std::span<const std::uint8_t> data) {
    StreamAnalysis analysis{};
    analysis.input_path = std::move(input_path);
    analysis.codec_guess = "h265";
    analysis.size_bytes = data.size();

    const auto units = bitstream::scan_annex_b(data);
    analysis.nals.reserve(units.size());
    std::map<std::uint32_t, bitstream::H265PpsInfo> pps_by_id;

    for (std::size_t i = 0; i < units.size(); ++i) {
        const auto& unit = units[i];
        NalAnalysis nal{};
        nal.index = i;
        nal.unit = unit;
        nal.h265 = H265NalAnalysis{};

        if (unit.payload_size >= 2) {
            nal.h265->header = bitstream::parse_h265_nal_header(data[unit.payload_offset], data[unit.payload_offset + 1]);
            if (nal.h265->header.nal_unit_type == bitstream::H265NalType::Vps) {
                ++analysis.summary.vps_count;
                const auto payload = data.subspan(unit.payload_offset, unit.payload_size);
                const auto vps = bitstream::parse_h265_vps(payload);
                if (vps.status.is_ok() && vps.info.has_value()) {
                    nal.h265->vps = *vps.info;
                    analysis.summary.active_h265_vps = *vps.info;
                } else {
                    nal.h265->vps_parse_error = vps.status.message();
                    add_parse_error(analysis.summary, &ParseErrorStats::vps);
                }
            } else if (nal.h265->header.nal_unit_type == bitstream::H265NalType::Sps) {
                ++analysis.summary.sps_count;
                const auto payload = data.subspan(unit.payload_offset, unit.payload_size);
                const auto sps = bitstream::parse_h265_sps(payload);
                if (sps.status.is_ok() && sps.info.has_value()) {
                    nal.h265->sps = *sps.info;
                    analysis.summary.active_h265_sps = *sps.info;
                } else {
                    nal.h265->sps_parse_error = sps.status.message();
                    add_parse_error(analysis.summary, &ParseErrorStats::sps);
                }
            } else if (nal.h265->header.nal_unit_type == bitstream::H265NalType::Pps) {
                ++analysis.summary.pps_count;
                const auto payload = data.subspan(unit.payload_offset, unit.payload_size);
                const auto pps = bitstream::parse_h265_pps(payload);
                if (pps.status.is_ok() && pps.info.has_value()) {
                    nal.h265->pps = *pps.info;
                    pps_by_id[pps.info->pic_parameter_set_id] = *pps.info;
                } else {
                    nal.h265->pps_parse_error = pps.status.message();
                    add_parse_error(analysis.summary, &ParseErrorStats::pps);
                }
            } else if (bitstream::h265_nal_type_is_vcl(nal.h265->header.nal_unit_type)) {
                const auto payload = data.subspan(unit.payload_offset, unit.payload_size);
                const auto slice_prefix = bitstream::parse_h265_slice_header(payload, nal.h265->header.nal_unit_type, std::nullopt);
                std::optional<std::uint8_t> extra_slice_header_bits;
                if (slice_prefix.status.is_ok() && slice_prefix.info.has_value()) {
                    const auto pps = pps_by_id.find(slice_prefix.info->slice_pic_parameter_set_id);
                    if (pps != pps_by_id.end()) {
                        extra_slice_header_bits = pps->second.num_extra_slice_header_bits;
                    }
                }

                const auto slice = bitstream::parse_h265_slice_header(
                    payload,
                    nal.h265->header.nal_unit_type,
                    extra_slice_header_bits);
                if (slice.status.is_ok() && slice.info.has_value()) {
                    nal.h265->slice = *slice.info;
                    add_h265_slice_to_summary(analysis.summary, *slice.info);
                    add_h265_frame(analysis, nal);
                } else {
                    nal.h265->slice_parse_error = slice.status.message();
                    add_parse_error(analysis.summary, &ParseErrorStats::slice);
                }
            }
        }

        analysis.nals.push_back(std::move(nal));
    }

    build_gops(analysis);
    return analysis;
}

} // namespace streamview::analysis
