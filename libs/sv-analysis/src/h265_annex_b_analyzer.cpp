#include "streamview/analysis/stream_analysis.hpp"

#include <utility>

namespace streamview::analysis {

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
            } else if (nal.h265->header.nal_unit_type == bitstream::H265NalType::Pps) {
                ++analysis.summary.pps_count;
            } else if (bitstream::h265_nal_type_is_vcl(nal.h265->header.nal_unit_type)) {
                ++analysis.summary.slices.total;
            }
        }

        analysis.nals.push_back(std::move(nal));
    }

    return analysis;
}

} // namespace streamview::analysis
