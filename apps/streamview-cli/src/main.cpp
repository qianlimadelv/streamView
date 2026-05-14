#include "streamview/bitstream/annex_b.hpp"
#include "streamview/bitstream/h264_nal.hpp"
#include "streamview/bitstream/h264_pps.hpp"
#include "streamview/bitstream/h264_slice.hpp"
#include "streamview/bitstream/h264_sps.hpp"
#include "streamview/export/json_writer.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct AnalyzeOptions {
    std::string input_path;
    std::optional<std::string> json_output_path;
};

void print_usage(std::ostream& out) {
    out << "Usage:\n"
        << "  streamview analyze <input.h264> [--json <output.json>]\n"
        << "  streamview --help\n";
}

std::optional<AnalyzeOptions> parse_analyze_args(int argc, char** argv) {
    if (argc < 3) {
        return std::nullopt;
    }

    AnalyzeOptions options{.input_path = argv[2]};
    for (int i = 3; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--json") {
            if (i + 1 >= argc) {
                return std::nullopt;
            }
            options.json_output_path = argv[++i];
        } else {
            return std::nullopt;
        }
    }

    return options;
}

std::optional<std::vector<std::uint8_t>> read_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }

    return std::vector<std::uint8_t>(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>());
}

void write_analysis_json(std::ostream& out, const std::string& input_path, std::span<const std::uint8_t> data) {
    const auto units = streamview::bitstream::scan_annex_b(data);
    std::optional<streamview::bitstream::H264SpsInfo> active_sps;

    out << "{\n";
    out << "  \"format\": \"annex_b\",\n";
    out << "  \"codec_guess\": \"h264\",\n";
    out << "  \"input\": ";
    streamview::exporter::write_json_string(out, input_path);
    out << ",\n";
    out << "  \"size_bytes\": " << data.size() << ",\n";
    out << "  \"nal_count\": " << units.size() << ",\n";
    out << "  \"nals\": [\n";

    for (std::size_t i = 0; i < units.size(); ++i) {
        const auto& unit = units[i];
        const auto payload = data.subspan(unit.payload_offset, unit.payload_size);
        const auto header = streamview::bitstream::parse_h264_nal_header(data[unit.payload_offset]);
        out << "    {\n";
        out << "      \"index\": " << i << ",\n";
        out << "      \"start_code_offset\": " << unit.start_code_offset << ",\n";
        out << "      \"start_code_size\": " << unit.start_code_size << ",\n";
        out << "      \"payload_offset\": " << unit.payload_offset << ",\n";
        out << "      \"payload_size\": " << unit.payload_size << ",\n";
        out << "      \"h264\": {\n";
        out << "        \"forbidden_zero_bit\": " << static_cast<int>(header.forbidden_zero_bit) << ",\n";
        out << "        \"nal_ref_idc\": " << static_cast<int>(header.nal_ref_idc) << ",\n";
        out << "        \"nal_unit_type\": " << static_cast<int>(header.nal_unit_type) << ",\n";
        out << "        \"nal_unit_type_name\": ";
        streamview::exporter::write_json_string(out, streamview::bitstream::h264_nal_type_name(header.nal_unit_type));
        if (header.nal_unit_type == streamview::bitstream::H264NalType::Sps) {
            const auto sps = streamview::bitstream::parse_h264_sps(payload);
            if (sps.status.is_ok() && sps.info.has_value()) {
                active_sps = *sps.info;
                out << ",\n";
                out << "        \"sps\": {\n";
                out << "          \"profile_idc\": " << static_cast<int>(sps.info->profile_idc) << ",\n";
                out << "          \"constraint_flags\": " << static_cast<int>(sps.info->constraint_flags) << ",\n";
                out << "          \"level_idc\": " << static_cast<int>(sps.info->level_idc) << ",\n";
                out << "          \"seq_parameter_set_id\": " << sps.info->seq_parameter_set_id << ",\n";
                out << "          \"chroma_format_idc\": " << sps.info->chroma_format_idc << ",\n";
                out << "          \"bit_depth_luma\": " << static_cast<int>(sps.info->bit_depth_luma) << ",\n";
                out << "          \"bit_depth_chroma\": " << static_cast<int>(sps.info->bit_depth_chroma) << ",\n";
                out << "          \"log2_max_frame_num_minus4\": " << sps.info->log2_max_frame_num_minus4 << ",\n";
                out << "          \"width\": " << sps.info->width << ",\n";
                out << "          \"height\": " << sps.info->height << ",\n";
                out << "          \"frame_mbs_only_flag\": " << (sps.info->frame_mbs_only_flag ? "true" : "false") << ",\n";
                out << "          \"frame_cropping_flag\": " << (sps.info->frame_cropping_flag ? "true" : "false") << "\n";
                out << "        }\n";
            } else {
                out << ",\n";
                out << "        \"sps_parse_error\": ";
                streamview::exporter::write_json_string(out, sps.status.message());
                out << "\n";
            }
        } else if (header.nal_unit_type == streamview::bitstream::H264NalType::Pps) {
            const auto pps = streamview::bitstream::parse_h264_pps(payload);
            if (pps.status.is_ok() && pps.info.has_value()) {
                out << ",\n";
                out << "        \"pps\": {\n";
                out << "          \"pic_parameter_set_id\": " << pps.info->pic_parameter_set_id << ",\n";
                out << "          \"seq_parameter_set_id\": " << pps.info->seq_parameter_set_id << ",\n";
                out << "          \"entropy_coding_mode_flag\": " << (pps.info->entropy_coding_mode_flag ? "true" : "false") << ",\n";
                out << "          \"bottom_field_pic_order_in_frame_present_flag\": "
                    << (pps.info->bottom_field_pic_order_in_frame_present_flag ? "true" : "false") << ",\n";
                out << "          \"num_slice_groups_minus1\": " << pps.info->num_slice_groups_minus1 << "\n";
                out << "        }\n";
            } else {
                out << ",\n";
                out << "        \"pps_parse_error\": ";
                streamview::exporter::write_json_string(out, pps.status.message());
                out << "\n";
            }
        } else if (header.nal_unit_type == streamview::bitstream::H264NalType::CodedSliceNonIdr ||
                   header.nal_unit_type == streamview::bitstream::H264NalType::CodedSliceIdr) {
            if (active_sps.has_value()) {
                const auto slice = streamview::bitstream::parse_h264_slice_header(payload, active_sps->log2_max_frame_num_minus4);
                if (slice.status.is_ok() && slice.info.has_value()) {
                    out << ",\n";
                    out << "        \"slice\": {\n";
                    out << "          \"first_mb_in_slice\": " << slice.info->first_mb_in_slice << ",\n";
                    out << "          \"slice_type_raw\": " << slice.info->slice_type_raw << ",\n";
                    out << "          \"slice_type\": ";
                    streamview::exporter::write_json_string(out, streamview::bitstream::h264_slice_kind_name(slice.info->slice_kind));
                    out << ",\n";
                    out << "          \"slice_type_all_slices\": "
                        << (slice.info->slice_type_all_slices ? "true" : "false") << ",\n";
                    out << "          \"pic_parameter_set_id\": " << slice.info->pic_parameter_set_id << ",\n";
                    out << "          \"frame_num\": " << slice.info->frame_num << "\n";
                    out << "        }\n";
                } else {
                    out << ",\n";
                    out << "        \"slice_parse_error\": ";
                    streamview::exporter::write_json_string(out, slice.status.message());
                    out << "\n";
                }
            } else {
                out << ",\n";
                out << "        \"slice_parse_error\": \"missing active SPS\"\n";
            }
        } else {
            out << "\n";
        }
        out << "      }\n";
        out << "    }" << (i + 1 == units.size() ? "\n" : ",\n");
    }

    out << "  ]\n";
    out << "}\n";
}

int run_analyze(const AnalyzeOptions& options) {
    const auto data = read_file(options.input_path);
    if (!data.has_value()) {
        std::cerr << "streamview: failed to read input file: " << options.input_path << "\n";
        return 2;
    }

    if (options.json_output_path.has_value()) {
        std::ofstream output(*options.json_output_path, std::ios::binary);
        if (!output) {
            std::cerr << "streamview: failed to open JSON output file: " << *options.json_output_path << "\n";
            return 2;
        }
        write_analysis_json(output, options.input_path, *data);
        return 0;
    }

    write_analysis_json(std::cout, options.input_path, *data);
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2 || std::string_view(argv[1]) == "--help" || std::string_view(argv[1]) == "-h") {
        print_usage(std::cout);
        return argc < 2 ? 1 : 0;
    }

    const std::string_view command = argv[1];
    if (command == "analyze") {
        const auto options = parse_analyze_args(argc, argv);
        if (!options.has_value()) {
            print_usage(std::cerr);
            return 1;
        }
        return run_analyze(*options);
    }

    print_usage(std::cerr);
    return 1;
}
