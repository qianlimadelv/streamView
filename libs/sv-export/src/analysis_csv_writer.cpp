#include "streamview/export/analysis_csv_writer.hpp"

#include <cstdint>
#include <optional>
#include <string_view>

namespace streamview::exporter {
namespace {

void write_csv_cell(std::ostream& out, std::string_view value) {
    const bool needs_quotes = value.find_first_of(",\"\n\r") != std::string_view::npos;
    if (!needs_quotes) {
        out << value;
        return;
    }

    out << '"';
    for (const char ch : value) {
        if (ch == '"') {
            out << "\"\"";
        } else {
            out << ch;
        }
    }
    out << '"';
}

void write_csv_cell(std::ostream& out, const std::string& value) {
    write_csv_cell(out, std::string_view(value));
}

void write_csv_cell(std::ostream& out, const char* value) {
    write_csv_cell(out, std::string_view(value));
}

void write_csv_cell(std::ostream& out, std::uint64_t value) {
    out << value;
}

void write_csv_cell(std::ostream& out, std::uint32_t value) {
    out << value;
}

template <typename T>
void write_optional_cell(std::ostream& out, const std::optional<T>& value) {
    if (value.has_value()) {
        write_csv_cell(out, *value);
    }
}

template <typename T>
void write_separator(std::ostream& out, bool& first, const T& value) {
    if (!first) {
        out << ",";
    }
    first = false;
    write_csv_cell(out, value);
}

template <typename T>
void write_optional_separator(std::ostream& out, bool& first, const std::optional<T>& value) {
    if (!first) {
        out << ",";
    }
    first = false;
    write_optional_cell(out, value);
}

} // namespace

void write_analysis_csv(std::ostream& out, const analysis::StreamAnalysis& analysis) {
    out << "input,format,codec_guess,size_bytes,nal_count,frame_count,keyframe_count,gop_count,vps_count,sps_count,pps_count,parse_error_total,slice_total,slice_i,slice_p,slice_b,slice_sp,slice_si,active_sps_profile_idc,active_sps_level_idc,active_sps_width,active_sps_height,active_sps_id,active_h265_vps_profile_idc,active_h265_vps_level_idc,active_h265_vps_id,active_h265_sps_profile_idc,active_h265_sps_level_idc,active_h265_sps_vps_id,active_h265_sps_id,active_h265_sps_width,active_h265_sps_height,active_h265_log2_max_pic_order_cnt_lsb_minus4\n";

    bool first = true;
    write_separator(out, first, analysis.input_path);
    write_separator(out, first, analysis.format);
    write_separator(out, first, analysis.codec_guess);
    write_separator(out, first, static_cast<std::uint64_t>(analysis.size_bytes));
    write_separator(out, first, static_cast<std::uint64_t>(analysis.nals.size()));
    write_separator(out, first, static_cast<std::uint64_t>(analysis.summary.frame_count));
    write_separator(out, first, static_cast<std::uint64_t>(analysis.summary.keyframe_count));
    write_separator(out, first, static_cast<std::uint64_t>(analysis.summary.gop_count));
    write_separator(out, first, static_cast<std::uint64_t>(analysis.summary.vps_count));
    write_separator(out, first, static_cast<std::uint64_t>(analysis.summary.sps_count));
    write_separator(out, first, static_cast<std::uint64_t>(analysis.summary.pps_count));
    write_separator(out, first, static_cast<std::uint64_t>(analysis.summary.parse_errors.total));
    write_separator(out, first, static_cast<std::uint64_t>(analysis.summary.slices.total));
    write_separator(out, first, static_cast<std::uint64_t>(analysis.summary.slices.i));
    write_separator(out, first, static_cast<std::uint64_t>(analysis.summary.slices.p));
    write_separator(out, first, static_cast<std::uint64_t>(analysis.summary.slices.b));
    write_separator(out, first, static_cast<std::uint64_t>(analysis.summary.slices.sp));
    write_separator(out, first, static_cast<std::uint64_t>(analysis.summary.slices.si));
    write_optional_separator(out, first, analysis.summary.active_sps.has_value()
                                             ? std::optional<std::uint32_t>(analysis.summary.active_sps->profile_idc)
                                             : std::nullopt);
    write_optional_separator(out, first, analysis.summary.active_sps.has_value()
                                             ? std::optional<std::uint32_t>(analysis.summary.active_sps->level_idc)
                                             : std::nullopt);
    write_optional_separator(out, first, analysis.summary.active_sps.has_value()
                                             ? std::optional<std::uint64_t>(analysis.summary.active_sps->width)
                                             : std::nullopt);
    write_optional_separator(out, first, analysis.summary.active_sps.has_value()
                                             ? std::optional<std::uint64_t>(analysis.summary.active_sps->height)
                                             : std::nullopt);
    write_optional_separator(out, first, analysis.summary.active_sps.has_value()
                                             ? std::optional<std::uint64_t>(analysis.summary.active_sps->seq_parameter_set_id)
                                             : std::nullopt);
    write_optional_separator(out, first, analysis.summary.active_h265_vps.has_value()
                                             ? std::optional<std::uint32_t>(analysis.summary.active_h265_vps->profile_idc)
                                             : std::nullopt);
    write_optional_separator(out, first, analysis.summary.active_h265_vps.has_value()
                                             ? std::optional<std::uint32_t>(analysis.summary.active_h265_vps->level_idc)
                                             : std::nullopt);
    write_optional_separator(out, first, analysis.summary.active_h265_vps.has_value()
                                             ? std::optional<std::uint64_t>(analysis.summary.active_h265_vps->video_parameter_set_id)
                                             : std::nullopt);
    write_optional_separator(out, first, analysis.summary.active_h265_sps.has_value()
                                             ? std::optional<std::uint32_t>(analysis.summary.active_h265_sps->profile_idc)
                                             : std::nullopt);
    write_optional_separator(out, first, analysis.summary.active_h265_sps.has_value()
                                             ? std::optional<std::uint32_t>(analysis.summary.active_h265_sps->level_idc)
                                             : std::nullopt);
    write_optional_separator(out, first, analysis.summary.active_h265_sps.has_value()
                                             ? std::optional<std::uint64_t>(analysis.summary.active_h265_sps->video_parameter_set_id)
                                             : std::nullopt);
    write_optional_separator(out, first, analysis.summary.active_h265_sps.has_value()
                                             ? std::optional<std::uint64_t>(analysis.summary.active_h265_sps->seq_parameter_set_id)
                                             : std::nullopt);
    write_optional_separator(out, first, analysis.summary.active_h265_sps.has_value()
                                             ? std::optional<std::uint64_t>(analysis.summary.active_h265_sps->width)
                                             : std::nullopt);
    write_optional_separator(out, first, analysis.summary.active_h265_sps.has_value()
                                             ? std::optional<std::uint64_t>(analysis.summary.active_h265_sps->height)
                                             : std::nullopt);
    write_optional_separator(out, first, analysis.summary.active_h265_sps.has_value()
                                             ? std::optional<std::uint64_t>(analysis.summary.active_h265_sps->log2_max_pic_order_cnt_lsb_minus4)
                                             : std::nullopt);
    out << "\n";
}

} // namespace streamview::exporter
