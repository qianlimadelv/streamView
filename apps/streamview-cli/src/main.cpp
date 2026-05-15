#include "streamview/analysis/stream_analysis.hpp"
#include "streamview/export/analysis_json_writer.hpp"
#include "streamview/export/json_writer.hpp"
#if defined(STREAMVIEW_HAS_FFMPEG_DEMUX)
#include "streamview/demux/ffmpeg_h264_demuxer.hpp"
#endif

#include <cstdint>
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

struct AnalyzeOptions {
    std::string input_path;
    std::optional<std::string> json_output_path;
    streamview::exporter::AnalysisJsonMode json_mode{streamview::exporter::AnalysisJsonMode::Full};
    std::optional<std::size_t> nal_limit;
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

void print_usage(std::ostream& out) {
    out << "Usage:\n"
        << "  streamview analyze <input.h264> [--json <output.json>] [--json-mode full|summary] [--limit-nals <count>]\n"
        << "  streamview inspect <input.h264> --nal <index>|--frame <index>|--gop <index>\n"
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
            options.json_output_path = argv[++i];
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

std::optional<streamview::analysis::StreamAnalysis> analyze_input(const std::string& input_path) {
    const auto data = load_input_as_annex_b(input_path);
    if (!data.has_value()) {
        std::cerr << "streamview: failed to read input file: " << input_path << "\n";
        return std::nullopt;
    }

    if (is_h265_input(input_path)) {
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

int write_json_output(const AnalyzeOptions& options, const streamview::analysis::StreamAnalysis& analysis) {
    if (options.json_output_path.has_value()) {
        std::ofstream output(*options.json_output_path, std::ios::binary);
        if (!output) {
            std::cerr << "streamview: failed to open JSON output file: " << *options.json_output_path << "\n";
            return 2;
        }
        streamview::exporter::write_analysis_json(
            output,
            analysis,
            streamview::exporter::AnalysisJsonOptions{
                .mode = options.json_mode,
                .nal_limit = options.nal_limit,
            });
        return 0;
    }

    write_text_summary(std::cout, analysis);
    return 0;
}

int run_analyze(const AnalyzeOptions& options) {
    const auto analysis = analyze_input(options.input_path);
    if (!analysis.has_value()) {
        return 2;
    }

    return write_json_output(options, *analysis);
}

void write_nal_inspect_json(std::ostream& out,
                            const streamview::analysis::StreamAnalysis& analysis,
                            std::size_t nal_index) {
    const auto& nal = analysis.nals[nal_index];
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
    out << "    \"payload_size\": " << nal.unit.payload_size;

    if (nal.h264.has_value()) {
        out << ",\n";
        out << "    \"codec\": \"h264\",\n";
        out << "    \"nal_unit_type\": " << static_cast<int>(nal.h264->header.nal_unit_type) << ",\n";
        out << "    \"nal_unit_type_name\": ";
        streamview::exporter::write_json_string(out, streamview::bitstream::h264_nal_type_name(nal.h264->header.nal_unit_type));
        if (nal.h264->sps.has_value()) {
            out << ",\n";
            out << "    \"width\": " << nal.h264->sps->width << ",\n";
            out << "    \"height\": " << nal.h264->sps->height;
        } else if (nal.h264->slice.has_value()) {
            out << ",\n";
            out << "    \"slice_type\": ";
            streamview::exporter::write_json_string(out, streamview::bitstream::h264_slice_kind_name(nal.h264->slice->slice_kind));
            out << ",\n";
            out << "    \"frame_num\": " << nal.h264->slice->frame_num;
        }
    } else if (nal.h265.has_value()) {
        out << ",\n";
        out << "    \"codec\": \"h265\",\n";
        out << "    \"nal_unit_type\": " << static_cast<int>(nal.h265->header.nal_unit_type) << ",\n";
        out << "    \"nal_unit_type_name\": ";
        streamview::exporter::write_json_string(out, streamview::bitstream::h265_nal_type_name(nal.h265->header.nal_unit_type));
        if (nal.h265->sps.has_value()) {
            out << ",\n";
            out << "    \"width\": " << nal.h265->sps->width << ",\n";
            out << "    \"height\": " << nal.h265->sps->height;
        } else if (nal.h265->slice.has_value()) {
            out << ",\n";
            out << "    \"slice_pic_parameter_set_id\": " << nal.h265->slice->slice_pic_parameter_set_id;
            if (nal.h265->slice->slice_type_present) {
                out << ",\n";
                out << "    \"slice_type\": ";
                streamview::exporter::write_json_string(out, streamview::bitstream::h265_slice_kind_name(nal.h265->slice->slice_kind));
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
    out << "    \"codec\": ";
    streamview::exporter::write_json_string(out, frame.codec);
    out << ",\n";
    out << "    \"frame_type\": ";
    streamview::exporter::write_json_string(out, frame.frame_type);
    out << ",\n";
    out << "    \"is_keyframe\": " << (frame.is_keyframe ? "true" : "false") << ",\n";
    out << "    \"size_bytes\": " << frame.size_bytes << ",\n";
    out << "    \"first_payload_offset\": " << frame.first_payload_offset << ",\n";
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

} // namespace

int main(int argc, char** argv) {
    if (argc < 2 || std::string_view(argv[1]) == "--help" || std::string_view(argv[1]) == "-h") {
        print_usage(std::cout);
        return argc < 2 ? 1 : 0;
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

    std::cerr << "streamview: unknown command: " << command << "\n";
    print_usage(std::cerr);
    return 1;
}
