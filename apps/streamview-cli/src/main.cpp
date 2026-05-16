#include "streamview/analysis/stream_analysis.hpp"
#include "streamview/analysis/validation.hpp"
#include "streamview/bitstream/rbsp.hpp"
#include "streamview/export/analysis_json_writer.hpp"
#include "streamview/export/json_writer.hpp"
#if defined(STREAMVIEW_HAS_FFMPEG_DEMUX)
#include "streamview/demux/ffmpeg_h264_demuxer.hpp"
#endif

#ifndef STREAMVIEW_VERSION
#define STREAMVIEW_VERSION "0.0.0"
#endif

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <filesystem>
#include <charconv>
#include <string>
#include <string_view>
#include <vector>

namespace {

using streamview::analysis::ValidationIssue;

enum class OutputFormat {
    Text,
    Json,
};

enum class CodecOverride {
    Auto,
    H264,
    H265,
};

enum class DumpFormat {
    Hex,
    Payload,
    Rbsp,
};

struct AnalyzeOptions {
    std::string input_path;
    OutputFormat output_format{OutputFormat::Text};
    std::optional<std::string> output_path;
    streamview::exporter::AnalysisJsonMode json_mode{streamview::exporter::AnalysisJsonMode::Full};
    std::optional<std::size_t> nal_limit;
    CodecOverride codec{CodecOverride::Auto};
};

struct ParseAnalyzeArgsResult {
    std::optional<AnalyzeOptions> options;
    std::string error;
};

struct InspectOptions {
    std::string input_path;
    std::optional<std::size_t> nal_index;
    std::optional<std::size_t> frame_index;
    std::optional<std::size_t> gop_index;
};

struct ParseInspectArgsResult {
    std::optional<InspectOptions> options;
    std::string error;
};

struct ErrorsOptions {
    std::string input_path;
    bool json_output{};
};

struct ParseErrorsArgsResult {
    std::optional<ErrorsOptions> options;
    std::string error;
};

struct ValidateOptions {
    std::string input_path;
    bool json_output{};
};

struct ParseValidateArgsResult {
    std::optional<ValidateOptions> options;
    std::string error;
};

struct DumpOptions {
    std::string input_path;
    std::size_t nal_index{};
    DumpFormat format{DumpFormat::Hex};
    std::optional<std::string> output_path;
};

struct ParseDumpArgsResult {
    std::optional<DumpOptions> options;
    std::string error;
};

void print_usage(std::ostream& out) {
    out << "Usage:\n"
        << "  streamview analyze <input> [--format text|json] [--output <path|->] [--codec auto|h264|h265]\n"
        << "                    [--json <output.json>] [--json-mode full|summary] [--limit-nals <count>]\n"
        << "  streamview inspect <input.h264> --nal <index>|--frame <index>|--gop <index>\n"
        << "  streamview errors <input.h264> [--json]\n"
        << "  streamview validate <input.h264> [--json]\n"
        << "  streamview dump <input.h264> --nal <index> [--format hex|payload|rbsp] [--output <path|->]\n"
        << "  streamview --help\n";
}

std::optional<streamview::exporter::AnalysisJsonMode> parse_json_mode(std::string_view value) {
    if (value == "full") {
        return streamview::exporter::AnalysisJsonMode::Full;
    }
    if (value == "summary") {
        return streamview::exporter::AnalysisJsonMode::Summary;
    }
    return std::nullopt;
}

std::optional<OutputFormat> parse_output_format(std::string_view value) {
    if (value == "text") {
        return OutputFormat::Text;
    }
    if (value == "json") {
        return OutputFormat::Json;
    }
    return std::nullopt;
}

std::optional<CodecOverride> parse_codec_override(std::string_view value) {
    if (value == "auto") {
        return CodecOverride::Auto;
    }
    if (value == "h264") {
        return CodecOverride::H264;
    }
    if (value == "h265") {
        return CodecOverride::H265;
    }
    return std::nullopt;
}

std::optional<DumpFormat> parse_dump_format(std::string_view value) {
    if (value == "hex") {
        return DumpFormat::Hex;
    }
    if (value == "payload") {
        return DumpFormat::Payload;
    }
    if (value == "rbsp") {
        return DumpFormat::Rbsp;
    }
    return std::nullopt;
}

std::optional<std::size_t> parse_size_arg(std::string_view value) {
    std::size_t parsed{};
    const auto* begin = value.data();
    const auto* end = value.data() + value.size();
    const auto result = std::from_chars(begin, end, parsed);
    if (result.ec != std::errc{} || result.ptr != end) {
        return std::nullopt;
    }
    return parsed;
}

ParseAnalyzeArgsResult parse_analyze_args(int argc, char** argv) {
    if (argc < 3) {
        return {.error = "missing input path"};
    }

    AnalyzeOptions options{.input_path = argv[2]};
    for (int i = 3; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--json") {
            if (i + 1 >= argc) {
                return {.error = "--json requires an output path"};
            }
            options.output_format = OutputFormat::Json;
            options.output_path = argv[++i];
        } else if (arg == "--format") {
            if (i + 1 >= argc) {
                return {.error = "--format requires text or json"};
            }
            const auto format = parse_output_format(argv[++i]);
            if (!format.has_value()) {
                return {.error = "--format must be text or json"};
            }
            options.output_format = *format;
        } else if (arg == "--output") {
            if (i + 1 >= argc) {
                return {.error = "--output requires a path or -"};
            }
            options.output_path = argv[++i];
        } else if (arg == "--codec") {
            if (i + 1 >= argc) {
                return {.error = "--codec requires auto, h264, or h265"};
            }
            const auto codec = parse_codec_override(argv[++i]);
            if (!codec.has_value()) {
                return {.error = "--codec must be auto, h264, or h265"};
            }
            options.codec = *codec;
        } else if (arg == "--json-mode") {
            if (i + 1 >= argc) {
                return {.error = "--json-mode requires full or summary"};
            }
            const auto mode = parse_json_mode(argv[++i]);
            if (!mode.has_value()) {
                return {.error = "--json-mode must be full or summary"};
            }
            options.json_mode = *mode;
        } else if (arg == "--limit-nals") {
            if (i + 1 >= argc) {
                return {.error = "--limit-nals requires a non-negative integer"};
            }
            const auto limit = parse_size_arg(argv[++i]);
            if (!limit.has_value()) {
                return {.error = "--limit-nals requires a non-negative integer"};
            }
            options.nal_limit = *limit;
        } else {
            return {.error = "unknown analyze option: " + std::string(arg)};
        }
    }

    return {.options = std::move(options)};
}

ParseInspectArgsResult parse_inspect_args(int argc, char** argv) {
    if (argc < 5) {
        return {.error = "inspect requires an input path and one selector"};
    }

    InspectOptions options{.input_path = argv[2]};
    int selector_count = 0;
    for (int i = 3; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--nal") {
            if (i + 1 >= argc) {
                return {.error = "--nal requires a non-negative integer"};
            }
            const auto index = parse_size_arg(argv[++i]);
            if (!index.has_value()) {
                return {.error = "--nal requires a non-negative integer"};
            }
            options.nal_index = index;
            ++selector_count;
        } else if (arg == "--frame") {
            if (i + 1 >= argc) {
                return {.error = "--frame requires a non-negative integer"};
            }
            const auto index = parse_size_arg(argv[++i]);
            if (!index.has_value()) {
                return {.error = "--frame requires a non-negative integer"};
            }
            options.frame_index = index;
            ++selector_count;
        } else if (arg == "--gop") {
            if (i + 1 >= argc) {
                return {.error = "--gop requires a non-negative integer"};
            }
            const auto index = parse_size_arg(argv[++i]);
            if (!index.has_value()) {
                return {.error = "--gop requires a non-negative integer"};
            }
            options.gop_index = index;
            ++selector_count;
        } else {
            return {.error = "unknown inspect option: " + std::string(arg)};
        }
    }

    if (selector_count != 1) {
        return {.error = "inspect requires exactly one selector: --nal, --frame, or --gop"};
    }
    return {.options = std::move(options)};
}

ParseErrorsArgsResult parse_errors_args(int argc, char** argv) {
    if (argc < 3) {
        return {.error = "errors requires an input path"};
    }

    ErrorsOptions options{.input_path = argv[2]};
    for (int i = 3; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--json") {
            options.json_output = true;
        } else {
            return {.error = "unknown errors option: " + std::string(arg)};
        }
    }
    return {.options = std::move(options)};
}

ParseValidateArgsResult parse_validate_args(int argc, char** argv) {
    if (argc < 3) {
        return {.error = "validate requires an input path"};
    }

    ValidateOptions options{.input_path = argv[2]};
    for (int i = 3; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--json") {
            options.json_output = true;
        } else {
            return {.error = "unknown validate option: " + std::string(arg)};
        }
    }
    return {.options = std::move(options)};
}

ParseDumpArgsResult parse_dump_args(int argc, char** argv) {
    if (argc < 5) {
        return {.error = "dump requires an input path and --nal"};
    }

    DumpOptions options{.input_path = argv[2]};
    bool has_nal = false;
    for (int i = 3; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--nal") {
            if (i + 1 >= argc) {
                return {.error = "--nal requires a non-negative integer"};
            }
            const auto index = parse_size_arg(argv[++i]);
            if (!index.has_value()) {
                return {.error = "--nal requires a non-negative integer"};
            }
            options.nal_index = *index;
            has_nal = true;
        } else if (arg == "--format") {
            if (i + 1 >= argc) {
                return {.error = "--format requires hex, payload, or rbsp"};
            }
            const auto format = parse_dump_format(argv[++i]);
            if (!format.has_value()) {
                return {.error = "--format must be hex, payload, or rbsp"};
            }
            options.format = *format;
        } else if (arg == "--output") {
            if (i + 1 >= argc) {
                return {.error = "--output requires a path or -"};
            }
            options.output_path = argv[++i];
        } else {
            return {.error = "unknown dump option: " + std::string(arg)};
        }
    }

    if (!has_nal) {
        return {.error = "dump requires --nal"};
    }
    return {.options = std::move(options)};
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

bool has_extension(const std::string& path, std::string_view extension) {
    return std::filesystem::path(path).extension() == extension;
}

bool is_h265_input(const std::string& path) {
    return has_extension(path, ".h265") || has_extension(path, ".265");
}

std::optional<std::vector<std::uint8_t>> load_input_as_annex_b(const std::string& path) {
    if (has_extension(path, ".h264") || has_extension(path, ".264")) {
        return read_file(path);
    }

    if (has_extension(path, ".mp4")) {
#if defined(STREAMVIEW_HAS_FFMPEG_DEMUX)
        const auto demux = streamview::demux::demux_h264_to_annex_b(path);
        if (!demux.status.is_ok()) {
            std::cerr << "streamview: failed to demux MP4: " << demux.status.message() << "\n";
            return std::nullopt;
        }
        return demux.annex_b;
#else
        std::cerr << "streamview: MP4 input requires FFmpeg development libraries at build time\n";
        return std::nullopt;
#endif
    }

    return read_file(path);
}

bool should_analyze_h265(const std::string& input_path, CodecOverride codec) {
    if (codec == CodecOverride::H265) {
        return true;
    }
    if (codec == CodecOverride::H264) {
        return false;
    }
    return is_h265_input(input_path);
}

std::optional<streamview::analysis::StreamAnalysis> analyze_input(const std::string& input_path,
                                                                  CodecOverride codec = CodecOverride::Auto) {
    const auto data = load_input_as_annex_b(input_path);
    if (!data.has_value()) {
        std::cerr << "streamview: failed to read input file: " << input_path << "\n";
        return std::nullopt;
    }

    if (should_analyze_h265(input_path, codec)) {
        return streamview::analysis::analyze_h265_annex_b(input_path, *data);
    }
    return streamview::analysis::analyze_h264_annex_b(input_path, *data);
}

void write_text_summary(std::ostream& out, const streamview::analysis::StreamAnalysis& analysis) {
    out << "StreamView analysis summary\n";
    out << "Input: " << analysis.input_path << "\n";
    out << "Format: " << analysis.format << "\n";
    out << "Codec: " << analysis.codec_guess << "\n";
    out << "Size bytes: " << analysis.size_bytes << "\n";
    out << "NAL units: " << analysis.nals.size() << "\n";
    out << "Frames: " << analysis.summary.frame_count << "\n";
    out << "Keyframes: " << analysis.summary.keyframe_count << "\n";
    out << "GOPs: " << analysis.summary.gop_count << "\n";
    out << "Parameter sets: VPS=" << analysis.summary.vps_count
        << ", SPS=" << analysis.summary.sps_count
        << ", PPS=" << analysis.summary.pps_count << "\n";

    if (analysis.summary.active_sps.has_value()) {
        out << "Resolution: " << analysis.summary.active_sps->width << "x" << analysis.summary.active_sps->height
            << "\n";
        out << "Profile/level: " << static_cast<int>(analysis.summary.active_sps->profile_idc)
            << "/" << static_cast<int>(analysis.summary.active_sps->level_idc) << "\n";
    } else if (analysis.summary.active_h265_sps.has_value()) {
        out << "Resolution: " << analysis.summary.active_h265_sps->width << "x"
            << analysis.summary.active_h265_sps->height << "\n";
        out << "Profile/level: " << static_cast<int>(analysis.summary.active_h265_sps->profile_idc)
            << "/" << static_cast<int>(analysis.summary.active_h265_sps->level_idc) << "\n";
    } else {
        out << "Resolution: unknown\n";
        out << "Profile/level: unknown\n";
    }

    out << "Slices: total=" << analysis.summary.slices.total
        << ", I=" << analysis.summary.slices.i
        << ", P=" << analysis.summary.slices.p
        << ", B=" << analysis.summary.slices.b
        << ", SP=" << analysis.summary.slices.sp
        << ", SI=" << analysis.summary.slices.si << "\n";
    out << "Parse errors: total=" << analysis.summary.parse_errors.total
        << ", VPS=" << analysis.summary.parse_errors.vps
        << ", SPS=" << analysis.summary.parse_errors.sps
        << ", PPS=" << analysis.summary.parse_errors.pps
        << ", slice=" << analysis.summary.parse_errors.slice << "\n";
}

int write_analyze_output(const AnalyzeOptions& options, const streamview::analysis::StreamAnalysis& analysis) {
    std::ofstream file_output;
    std::ostream* output = &std::cout;
    if (options.output_path.has_value() && *options.output_path != "-") {
        file_output.open(*options.output_path, std::ios::binary);
        output = &file_output;
        if (!file_output) {
            std::cerr << "streamview: failed to open output file: " << *options.output_path << "\n";
            return 2;
        }
    }

    if (options.output_format == OutputFormat::Json) {
        streamview::exporter::write_analysis_json(
            *output,
            analysis,
            streamview::exporter::AnalysisJsonOptions{
                .mode = options.json_mode,
                .nal_limit = options.nal_limit,
            });
        return 0;
    }

    write_text_summary(*output, analysis);
    return 0;
}

int run_analyze(const AnalyzeOptions& options) {
    const auto analysis = analyze_input(options.input_path, options.codec);
    if (!analysis.has_value()) {
        return 2;
    }

    return write_analyze_output(options, *analysis);
}

std::optional<std::string> nal_parse_error_message(const streamview::analysis::NalAnalysis& nal);

std::vector<std::size_t> find_frame_indices_for_nal(const streamview::analysis::StreamAnalysis& analysis,
                                                    std::size_t nal_index) {
    std::vector<std::size_t> frame_indices;
    for (const auto& frame : analysis.frames) {
        for (const auto frame_nal_index : frame.nal_indices) {
            if (frame_nal_index == nal_index) {
                frame_indices.push_back(frame.index);
                break;
            }
        }
    }
    return frame_indices;
}

std::optional<std::size_t> find_gop_index_for_frame(const streamview::analysis::StreamAnalysis& analysis,
                                                    std::size_t frame_index) {
    for (const auto& gop : analysis.gops) {
        if (frame_index >= gop.start_frame_index && frame_index <= gop.end_frame_index) {
            return gop.index;
        }
    }
    return std::nullopt;
}

void write_size_array(std::ostream& out, const std::vector<std::size_t>& values) {
    out << "[";
    for (std::size_t i = 0; i < values.size(); ++i) {
        out << values[i] << (i + 1 == values.size() ? "" : ", ");
    }
    out << "]";
}

void write_nal_inspect_json(std::ostream& out,
                            const streamview::analysis::StreamAnalysis& analysis,
                            std::size_t nal_index) {
    const auto& nal = analysis.nals[nal_index];
    const auto frame_indices = find_frame_indices_for_nal(analysis, nal_index);
    const auto parse_error = nal_parse_error_message(nal);
    out << "{\n";
    out << "  \"input\": ";
    streamview::exporter::write_json_string(out, analysis.input_path);
    out << ",\n";
    out << "  \"codec_guess\": ";
    streamview::exporter::write_json_string(out, analysis.codec_guess);
    out << ",\n";
    out << "  \"nal_count\": " << analysis.nals.size() << ",\n";
    out << "  \"nal\": {\n";
    out << "    \"index\": " << nal.index << ",\n";
    out << "    \"start_code_offset\": " << nal.unit.start_code_offset << ",\n";
    out << "    \"start_code_size\": " << nal.unit.start_code_size << ",\n";
    out << "    \"payload_offset\": " << nal.unit.payload_offset << ",\n";
    out << "    \"payload_size\": " << nal.unit.payload_size << ",\n";
    out << "    \"frame_indices\": ";
    write_size_array(out, frame_indices);
    if (parse_error.has_value()) {
        out << ",\n";
        out << "    \"parse_error\": ";
        streamview::exporter::write_json_string(out, *parse_error);
    }

    if (nal.h264.has_value()) {
        out << ",\n";
        out << "    \"codec\": \"h264\",\n";
        out << "    \"nal_unit_type\": " << static_cast<int>(nal.h264->header.nal_unit_type) << ",\n";
        out << "    \"nal_unit_type_name\": ";
        streamview::exporter::write_json_string(out, streamview::bitstream::h264_nal_type_name(nal.h264->header.nal_unit_type));
        if (nal.h264->sps.has_value()) {
            out << ",\n";
            out << "    \"profile_idc\": " << static_cast<int>(nal.h264->sps->profile_idc) << ",\n";
            out << "    \"level_idc\": " << static_cast<int>(nal.h264->sps->level_idc) << ",\n";
            out << "    \"seq_parameter_set_id\": " << nal.h264->sps->seq_parameter_set_id << ",\n";
            out << "    \"width\": " << nal.h264->sps->width << ",\n";
            out << "    \"height\": " << nal.h264->sps->height;
        } else if (nal.h264->pps.has_value()) {
            out << ",\n";
            out << "    \"pic_parameter_set_id\": " << nal.h264->pps->pic_parameter_set_id << ",\n";
            out << "    \"seq_parameter_set_id\": " << nal.h264->pps->seq_parameter_set_id << ",\n";
            out << "    \"entropy_coding_mode_flag\": "
                << (nal.h264->pps->entropy_coding_mode_flag ? "true" : "false");
        } else if (nal.h264->slice.has_value()) {
            out << ",\n";
            out << "    \"slice_type\": ";
            streamview::exporter::write_json_string(out, streamview::bitstream::h264_slice_kind_name(nal.h264->slice->slice_kind));
            out << ",\n";
            out << "    \"slice_type_raw\": " << nal.h264->slice->slice_type_raw << ",\n";
            out << "    \"pic_parameter_set_id\": " << nal.h264->slice->pic_parameter_set_id << ",\n";
            out << "    \"frame_num\": " << nal.h264->slice->frame_num;
            if (nal.h264->slice->idr_pic_id_present) {
                out << ",\n";
                out << "    \"idr_pic_id\": " << nal.h264->slice->idr_pic_id;
            }
            if (nal.h264->slice->pic_order_cnt_lsb_present) {
                out << ",\n";
                out << "    \"pic_order_cnt_lsb\": " << nal.h264->slice->pic_order_cnt_lsb;
            }
        }
    } else if (nal.h265.has_value()) {
        out << ",\n";
        out << "    \"codec\": \"h265\",\n";
        out << "    \"nal_unit_type\": " << static_cast<int>(nal.h265->header.nal_unit_type) << ",\n";
        out << "    \"nal_unit_type_name\": ";
        streamview::exporter::write_json_string(out, streamview::bitstream::h265_nal_type_name(nal.h265->header.nal_unit_type));
        if (nal.h265->vps.has_value()) {
            out << ",\n";
            out << "    \"profile_idc\": " << static_cast<int>(nal.h265->vps->profile_idc) << ",\n";
            out << "    \"level_idc\": " << static_cast<int>(nal.h265->vps->level_idc) << ",\n";
            out << "    \"video_parameter_set_id\": " << nal.h265->vps->video_parameter_set_id;
        } else if (nal.h265->sps.has_value()) {
            out << ",\n";
            out << "    \"profile_idc\": " << static_cast<int>(nal.h265->sps->profile_idc) << ",\n";
            out << "    \"level_idc\": " << static_cast<int>(nal.h265->sps->level_idc) << ",\n";
            out << "    \"video_parameter_set_id\": " << nal.h265->sps->video_parameter_set_id << ",\n";
            out << "    \"seq_parameter_set_id\": " << nal.h265->sps->seq_parameter_set_id << ",\n";
            out << "    \"log2_max_pic_order_cnt_lsb_minus4\": "
                << nal.h265->sps->log2_max_pic_order_cnt_lsb_minus4 << ",\n";
            out << "    \"width\": " << nal.h265->sps->width << ",\n";
            out << "    \"height\": " << nal.h265->sps->height;
        } else if (nal.h265->pps.has_value()) {
            out << ",\n";
            out << "    \"pic_parameter_set_id\": " << nal.h265->pps->pic_parameter_set_id << ",\n";
            out << "    \"seq_parameter_set_id\": " << nal.h265->pps->seq_parameter_set_id << ",\n";
            out << "    \"num_extra_slice_header_bits\": " << static_cast<int>(nal.h265->pps->num_extra_slice_header_bits);
        } else if (nal.h265->slice.has_value()) {
            out << ",\n";
            out << "    \"slice_pic_parameter_set_id\": " << nal.h265->slice->slice_pic_parameter_set_id;
            if (nal.h265->slice->slice_type_present) {
                out << ",\n";
                out << "    \"slice_type\": ";
                streamview::exporter::write_json_string(out, streamview::bitstream::h265_slice_kind_name(nal.h265->slice->slice_kind));
            }
            if (nal.h265->slice->dependent_slice_segment_flag_present) {
                out << ",\n";
                out << "    \"dependent_slice_segment_flag\": "
                    << (nal.h265->slice->dependent_slice_segment_flag ? "true" : "false");
            }
            if (nal.h265->slice->pic_order_cnt_lsb_present) {
                out << ",\n";
                out << "    \"pic_order_cnt_lsb\": " << nal.h265->slice->pic_order_cnt_lsb;
            }
            if (nal.h265->slice->pic_output_flag_present) {
                out << ",\n";
                out << "    \"pic_output_flag\": "
                    << (nal.h265->slice->pic_output_flag ? "true" : "false");
            }
        }
    }

    out << "\n";
    out << "  }\n";
    out << "}\n";
}

void write_frame_inspect_json(std::ostream& out,
                              const streamview::analysis::StreamAnalysis& analysis,
                              std::size_t frame_index) {
    const auto& frame = analysis.frames[frame_index];
    const auto gop_index = find_gop_index_for_frame(analysis, frame_index);
    out << "{\n";
    out << "  \"input\": ";
    streamview::exporter::write_json_string(out, analysis.input_path);
    out << ",\n";
    out << "  \"codec_guess\": ";
    streamview::exporter::write_json_string(out, analysis.codec_guess);
    out << ",\n";
    out << "  \"frame_count\": " << analysis.frames.size() << ",\n";
    out << "  \"frame\": {\n";
    out << "    \"index\": " << frame.index << ",\n";
    out << "    \"decode_order_index\": " << frame.decode_order_index << ",\n";
    out << "    \"codec\": ";
    streamview::exporter::write_json_string(out, frame.codec);
    out << ",\n";
    out << "    \"frame_type\": ";
    streamview::exporter::write_json_string(out, frame.frame_type);
    out << ",\n";
    out << "    \"is_keyframe\": " << (frame.is_keyframe ? "true" : "false") << ",\n";
    out << "    \"poc\": ";
    if (frame.poc.has_value()) {
        out << *frame.poc;
    } else {
        out << "null";
    }
    out << ",\n";
    out << "    \"size_bytes\": " << frame.size_bytes << ",\n";
    out << "    \"first_payload_offset\": " << frame.first_payload_offset << ",\n";
    out << "    \"gop_index\": ";
    if (gop_index.has_value()) {
        out << *gop_index;
    } else {
        out << "null";
    }
    out << ",\n";
    out << "    \"nal_indices\": [";
    for (std::size_t i = 0; i < frame.nal_indices.size(); ++i) {
        out << frame.nal_indices[i] << (i + 1 == frame.nal_indices.size() ? "" : ", ");
    }
    out << "]\n";
    out << "  }\n";
    out << "}\n";
}

void write_gop_inspect_json(std::ostream& out,
                            const streamview::analysis::StreamAnalysis& analysis,
                            std::size_t gop_index) {
    const auto& gop = analysis.gops[gop_index];
    out << "{\n";
    out << "  \"input\": ";
    streamview::exporter::write_json_string(out, analysis.input_path);
    out << ",\n";
    out << "  \"codec_guess\": ";
    streamview::exporter::write_json_string(out, analysis.codec_guess);
    out << ",\n";
    out << "  \"gop_count\": " << analysis.gops.size() << ",\n";
    out << "  \"gop\": {\n";
    out << "    \"index\": " << gop.index << ",\n";
    out << "    \"start_frame_index\": " << gop.start_frame_index << ",\n";
    out << "    \"end_frame_index\": " << gop.end_frame_index << ",\n";
    out << "    \"frame_count\": " << gop.frame_count << ",\n";
    out << "    \"keyframe_index\": " << gop.keyframe_index << ",\n";
    out << "    \"size_bytes\": " << gop.size_bytes << ",\n";
    out << "    \"starts_with_keyframe\": " << (gop.starts_with_keyframe ? "true" : "false") << "\n";
    out << "  }\n";
    out << "}\n";
}

int run_inspect(const InspectOptions& options) {
    const auto analysis = analyze_input(options.input_path);
    if (!analysis.has_value()) {
        return 2;
    }
    if (options.nal_index.has_value()) {
        if (*options.nal_index >= analysis->nals.size()) {
            std::cerr << "streamview: NAL index out of range: " << *options.nal_index
                      << " >= " << analysis->nals.size() << "\n";
            return 2;
        }
        write_nal_inspect_json(std::cout, *analysis, *options.nal_index);
        return 0;
    }
    if (options.frame_index.has_value()) {
        if (*options.frame_index >= analysis->frames.size()) {
            std::cerr << "streamview: frame index out of range: " << *options.frame_index
                      << " >= " << analysis->frames.size() << "\n";
            return 2;
        }
        write_frame_inspect_json(std::cout, *analysis, *options.frame_index);
        return 0;
    }
    if (options.gop_index.has_value()) {
        if (*options.gop_index >= analysis->gops.size()) {
            std::cerr << "streamview: GOP index out of range: " << *options.gop_index
                      << " >= " << analysis->gops.size() << "\n";
            return 2;
        }
        write_gop_inspect_json(std::cout, *analysis, *options.gop_index);
        return 0;
    }
    std::cerr << "streamview: inspect requires a selector\n";
    return 1;
}

std::optional<std::string> nal_parse_error_message(const streamview::analysis::NalAnalysis& nal) {
    if (nal.h264.has_value()) {
        if (nal.h264->sps_parse_error.has_value()) {
            return *nal.h264->sps_parse_error;
        }
        if (nal.h264->pps_parse_error.has_value()) {
            return *nal.h264->pps_parse_error;
        }
        if (nal.h264->slice_parse_error.has_value()) {
            return *nal.h264->slice_parse_error;
        }
    }
    if (nal.h265.has_value()) {
        if (nal.h265->vps_parse_error.has_value()) {
            return *nal.h265->vps_parse_error;
        }
        if (nal.h265->sps_parse_error.has_value()) {
            return *nal.h265->sps_parse_error;
        }
        if (nal.h265->pps_parse_error.has_value()) {
            return *nal.h265->pps_parse_error;
        }
        if (nal.h265->slice_parse_error.has_value()) {
            return *nal.h265->slice_parse_error;
        }
    }
    return std::nullopt;
}

void write_errors_text(std::ostream& out, const streamview::analysis::StreamAnalysis& analysis) {
    out << "Parse errors: " << analysis.summary.parse_errors.total << "\n";
    for (const auto& nal : analysis.nals) {
        const auto message = nal_parse_error_message(nal);
        if (!message.has_value()) {
            continue;
        }
        out << "NAL " << nal.index
            << " offset=" << nal.unit.payload_offset
            << " size=" << nal.unit.payload_size
            << " error=" << *message << "\n";
    }
}

void write_errors_json(std::ostream& out, const streamview::analysis::StreamAnalysis& analysis) {
    out << "{\n";
    out << "  \"input\": ";
    streamview::exporter::write_json_string(out, analysis.input_path);
    out << ",\n";
    out << "  \"codec_guess\": ";
    streamview::exporter::write_json_string(out, analysis.codec_guess);
    out << ",\n";
    out << "  \"parse_error_count\": " << analysis.summary.parse_errors.total << ",\n";
    out << "  \"errors\": [\n";
    bool first = true;
    for (const auto& nal : analysis.nals) {
        const auto message = nal_parse_error_message(nal);
        if (!message.has_value()) {
            continue;
        }
        if (!first) {
            out << ",\n";
        }
        first = false;
        out << "    {\n";
        out << "      \"nal_index\": " << nal.index << ",\n";
        out << "      \"payload_offset\": " << nal.unit.payload_offset << ",\n";
        out << "      \"payload_size\": " << nal.unit.payload_size << ",\n";
        out << "      \"message\": ";
        streamview::exporter::write_json_string(out, *message);
        out << "\n";
        out << "    }";
    }
    out << "\n";
    out << "  ]\n";
    out << "}\n";
}

int run_errors(const ErrorsOptions& options) {
    const auto analysis = analyze_input(options.input_path);
    if (!analysis.has_value()) {
        return 2;
    }
    if (options.json_output) {
        write_errors_json(std::cout, *analysis);
    } else {
        write_errors_text(std::cout, *analysis);
    }
    return analysis->summary.parse_errors.total == 0 ? 0 : 3;
}

std::vector<ValidationIssue> validate_analysis(const streamview::analysis::StreamAnalysis& analysis) {
    return streamview::analysis::validate_stream_analysis(analysis);
}

bool has_validation_errors(const std::vector<ValidationIssue>& issues) {
    for (const auto& issue : issues) {
        if (issue.severity == "error") {
            return true;
        }
    }
    return false;
}

void write_validate_text(std::ostream& out, const std::vector<ValidationIssue>& issues) {
    out << "Validation issues: " << issues.size() << "\n";
    for (const auto& issue : issues) {
        out << issue.severity << " " << issue.code << ": " << issue.message << "\n";
    }
}

void write_validate_json(std::ostream& out, const streamview::analysis::StreamAnalysis& analysis,
                         const std::vector<ValidationIssue>& issues) {
    out << "{\n";
    out << "  \"input\": ";
    streamview::exporter::write_json_string(out, analysis.input_path);
    out << ",\n";
    out << "  \"codec_guess\": ";
    streamview::exporter::write_json_string(out, analysis.codec_guess);
    out << ",\n";
    out << "  \"issue_count\": " << issues.size() << ",\n";
    out << "  \"issues\": [\n";
    for (std::size_t i = 0; i < issues.size(); ++i) {
        const auto& issue = issues[i];
        out << "    {\n";
        out << "      \"severity\": ";
        streamview::exporter::write_json_string(out, issue.severity);
        out << ",\n";
        out << "      \"code\": ";
        streamview::exporter::write_json_string(out, issue.code);
        out << ",\n";
        out << "      \"message\": ";
        streamview::exporter::write_json_string(out, issue.message);
        out << "\n";
        out << "    }" << (i + 1 == issues.size() ? "" : ",") << "\n";
    }
    out << "  ]\n";
    out << "}\n";
}

int run_validate(const ValidateOptions& options) {
    const auto analysis = analyze_input(options.input_path);
    if (!analysis.has_value()) {
        return 2;
    }

    const auto issues = validate_analysis(*analysis);
    if (options.json_output) {
        write_validate_json(std::cout, *analysis, issues);
    } else {
        write_validate_text(std::cout, issues);
    }
    return has_validation_errors(issues) ? 3 : 0;
}

void write_hex_dump(std::ostream& out, std::span<const std::uint8_t> bytes) {
    constexpr std::size_t bytes_per_line = 16;
    const auto old_flags = out.flags();
    const auto old_fill = out.fill();

    for (std::size_t offset = 0; offset < bytes.size(); offset += bytes_per_line) {
        out << std::hex << std::setw(8) << std::setfill('0') << offset << "  ";
        const auto line_size = std::min(bytes_per_line, bytes.size() - offset);
        for (std::size_t i = 0; i < bytes_per_line; ++i) {
            if (i < line_size) {
                out << std::setw(2) << static_cast<int>(bytes[offset + i]);
            } else {
                out << "  ";
            }
            out << (i == 7 ? "  " : " ");
        }
        out << " |";
        for (std::size_t i = 0; i < line_size; ++i) {
            const auto byte = bytes[offset + i];
            out << (byte >= 0x20 && byte <= 0x7e ? static_cast<char>(byte) : '.');
        }
        out << "|\n";
    }

    out.flags(old_flags);
    out.fill(old_fill);
}

int write_dump_output(const DumpOptions& options, std::span<const std::uint8_t> bytes) {
    std::ofstream file_output;
    std::ostream* output = &std::cout;
    if (options.output_path.has_value() && *options.output_path != "-") {
        file_output.open(*options.output_path, std::ios::binary);
        output = &file_output;
        if (!file_output) {
            std::cerr << "streamview: failed to open output file: " << *options.output_path << "\n";
            return 2;
        }
    }

    if (options.format == DumpFormat::Hex) {
        write_hex_dump(*output, bytes);
    } else {
        output->write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (!*output) {
            std::cerr << "streamview: failed to write dump output\n";
            return 2;
        }
    }
    return 0;
}

int run_dump(const DumpOptions& options) {
    const auto data = load_input_as_annex_b(options.input_path);
    if (!data.has_value()) {
        std::cerr << "streamview: failed to read input file: " << options.input_path << "\n";
        return 2;
    }

    const auto nals = streamview::bitstream::scan_annex_b(*data);
    if (options.nal_index >= nals.size()) {
        std::cerr << "streamview: NAL index out of range: " << options.nal_index
                  << " >= " << nals.size() << "\n";
        return 2;
    }

    const auto& nal = nals[options.nal_index];
    const auto payload = std::span<const std::uint8_t>(*data).subspan(nal.payload_offset, nal.payload_size);
    if (options.format == DumpFormat::Rbsp) {
        const auto rbsp = streamview::bitstream::nal_payload_to_rbsp(payload);
        return write_dump_output(options, rbsp);
    }

    return write_dump_output(options, payload);
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2 || std::string_view(argv[1]) == "--help" || std::string_view(argv[1]) == "-h") {
        print_usage(std::cout);
        return argc < 2 ? 1 : 0;
    }
    if (std::string_view(argv[1]) == "--version") {
        std::cout << "streamview " << STREAMVIEW_VERSION << "\n";
        return 0;
    }

    const std::string_view command = argv[1];
    if (command == "analyze") {
        const auto result = parse_analyze_args(argc, argv);
        if (!result.options.has_value()) {
            std::cerr << "streamview: " << result.error << "\n";
            print_usage(std::cerr);
            return 1;
        }
        return run_analyze(*result.options);
    }
    if (command == "inspect") {
        const auto result = parse_inspect_args(argc, argv);
        if (!result.options.has_value()) {
            std::cerr << "streamview: " << result.error << "\n";
            print_usage(std::cerr);
            return 1;
        }
        return run_inspect(*result.options);
    }
    if (command == "errors") {
        const auto result = parse_errors_args(argc, argv);
        if (!result.options.has_value()) {
            std::cerr << "streamview: " << result.error << "\n";
            print_usage(std::cerr);
            return 1;
        }
        return run_errors(*result.options);
    }
    if (command == "validate") {
        const auto result = parse_validate_args(argc, argv);
        if (!result.options.has_value()) {
            std::cerr << "streamview: " << result.error << "\n";
            print_usage(std::cerr);
            return 1;
        }
        return run_validate(*result.options);
    }
    if (command == "dump") {
        const auto result = parse_dump_args(argc, argv);
        if (!result.options.has_value()) {
            std::cerr << "streamview: " << result.error << "\n";
            print_usage(std::cerr);
            return 1;
        }
        return run_dump(*result.options);
    }

    std::cerr << "streamview: unknown command: " << command << "\n";
    print_usage(std::cerr);
    return 1;
}
