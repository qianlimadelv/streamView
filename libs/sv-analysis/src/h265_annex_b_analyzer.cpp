#include "streamview/analysis/stream_analysis.hpp"

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
    frame.frame_type = h265_frame_type_name(nal.h265->header.nal_unit_type);
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

} // namespace

StreamAnalysis analyze_h265_annex_b(std::string input_path, std::span<const std::uint8_t> data) {
    StreamAnalysis analysis{};
    analysis.input_path = std::move(input_path);
    analysis.codec_guess = "h265";
    analysis.size_bytes = data.size();

    const auto units = bitstream::scan_annex_b(data);
    analysis.nals.reserve(units.size());

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
            } else if (nal.h265->header.nal_unit_type == bitstream::H265NalType::Sps) {
                ++analysis.summary.sps_count;
                const auto payload = data.subspan(unit.payload_offset, unit.payload_size);
                const auto sps = bitstream::parse_h265_sps(payload);
                if (sps.status.is_ok() && sps.info.has_value()) {
                    nal.h265->sps = *sps.info;
                    analysis.summary.active_h265_sps = *sps.info;
                } else {
                    nal.h265->sps_parse_error = sps.status.message();
                }
            } else if (nal.h265->header.nal_unit_type == bitstream::H265NalType::Pps) {
                ++analysis.summary.pps_count;
            } else if (bitstream::h265_nal_type_is_vcl(nal.h265->header.nal_unit_type)) {
                ++analysis.summary.slices.total;
                const auto payload = data.subspan(unit.payload_offset, unit.payload_size);
                const auto slice = bitstream::parse_h265_slice_header(payload, nal.h265->header.nal_unit_type);
                if (slice.status.is_ok() && slice.info.has_value()) {
                    nal.h265->slice = *slice.info;
                } else {
                    nal.h265->slice_parse_error = slice.status.message();
                }
                add_h265_frame(analysis, nal);
            }
        }

        analysis.nals.push_back(std::move(nal));
    }

    build_gops(analysis);
    return analysis;
}

} // namespace streamview::analysis
