#include "streamview/analysis/stream_analysis.hpp"
#include "streamview/analysis/validation.hpp"
#include "streamview/export/analysis_csv_writer.hpp"
#include "streamview/bitstream/rbsp.hpp"
#include "streamview/export/analysis_json_writer.hpp"
#include "streamview/export/json_writer.hpp"
#if defined(STREAMVIEW_HAS_FFMPEG_DEMUX)
#include "streamview/demux/ffmpeg_mp4_demuxer.hpp"
#endif
#if defined(STREAMVIEW_HAS_FFMPEG_DECODE)
#include "streamview/decode/frame_decoder.hpp"
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
    Csv,
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

struct InputData {
    std::vector<std::uint8_t> bytes;
    std::optional<CodecOverride> codec_hint;
    std::optional<streamview::analysis::ContainerMetadata> container;
};

struct ParseDumpArgsResult {
    std::optional<DumpOptions> options;
    std::string error;
};

struct DecodeOptions {
    std::string input_path;
    std::size_t frame_index{};
    std::optional<std::string> thumbnail_path; // PPM output
    int thumbnail_max_dim{320};
    std::optional<std::string> mv_json_path;
    std::optional<std::string> block_layer; // qp|partition|intra|motion (HEVC, libde265)
    std::optional<std::string> block_out;   // PPM output for the block overlay
    std::optional<std::size_t> frames_start; // batch range mode: first decode index
    std::size_t frames_count{0};             //   number of frames to decode
    std::optional<std::string> thumb_dir;    //   output dir for <decode_index>.ppm
};

struct ParseDecodeArgsResult {
    std::optional<DecodeOptions> options;
    std::string error;
};

void print_usage(std::ostream& out) {
    out << "Usage:\n"
        << "  streamview analyze <input> [--format text|json|csv] [--output <path|->] [--codec auto|h264|h265]\n"
        << "                    [--json <output.json>] [--json-mode full|summary] [--limit-nals <count>]\n"
        << "  streamview inspect <input.h264> --nal <index>|--frame <index>|--gop <index>\n"
        << "  streamview errors <input.h264> [--json]\n"
        << "  streamview validate <input.h264> [--json]\n"
        << "  streamview dump <input.h264> --nal <index> [--format hex|payload|rbsp] [--output <path|->]\n"
        << "  streamview decode <input> --frame <index> [--thumb <out.ppm>] [--thumb-size <px>] [--mv-json <out.json>]\n"
        << "  streamview decode <input> --frames <start>:<count> --thumb-dir <dir> [--thumb-size <px>]  (batch thumbnails)\n"
        << "                    [--block-layer qp|partition|intra|motion --block-out <out.ppm>]  (HEVC, needs libde265)\n"
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
    if (value == "csv") {
        return OutputFormat::Csv;
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
                return {.error = "--format requires text, json, or csv"};
            }
            const auto format = parse_output_format(argv[++i]);
            if (!format.has_value()) {
                return {.error = "--format must be text, json, or csv"};
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

std::optional<CodecOverride> codec_from_extension(const std::string& path) {
    if (has_extension(path, ".h265") || has_extension(path, ".265")) {
        return CodecOverride::H265;
    }
    if (has_extension(path, ".h264") || has_extension(path, ".264")) {
        return CodecOverride::H264;
    }
    return std::nullopt;
}

// Containers demuxed to Annex B via FFmpeg (all use AVCC/HVCC extradata that the
// mp4toannexb/hevc filters handle). Extension-gated; codec is detected inside.
bool is_container_extension(const std::string& path) {
    return has_extension(path, ".mp4") || has_extension(path, ".mov") ||
           has_extension(path, ".m4v") || has_extension(path, ".mkv") ||
           has_extension(path, ".webm");
}

std::optional<InputData> load_input_data(const std::string& path) {
    if (const auto codec = codec_from_extension(path); codec.has_value()) {
        const auto data = read_file(path);
        if (!data.has_value()) {
            return std::nullopt;
        }
        return InputData{.bytes = std::move(*data), .codec_hint = codec, .container = std::nullopt};
    }

    if (is_container_extension(path)) {
#if defined(STREAMVIEW_HAS_FFMPEG_DEMUX)
        const auto demux = streamview::demux::demux_mp4_to_annex_b(path);
        if (!demux.status.is_ok()) {
            std::cerr << "streamview: failed to demux container: " << demux.status.message() << "\n";
            return std::nullopt;
        }
        streamview::analysis::ContainerMetadata container{};
        if (demux.stream.has_value()) {
            container.container_format_name = demux.stream->container_format_name;
            container.container_format_long_name = demux.stream->container_format_long_name;
            container.stream_codec_name = demux.stream->stream_codec_name;
            container.stream_codec_long_name = demux.stream->stream_codec_long_name;
            container.stream_index = demux.stream->stream_index;
            container.time_base_num = demux.stream->time_base_num;
            container.time_base_den = demux.stream->time_base_den;
            container.duration_ts = demux.stream->duration_ts;
            container.start_time_ts = demux.stream->start_time_ts;
            container.width = demux.stream->width;
            container.height = demux.stream->height;
            container.bit_rate = demux.stream->bit_rate;
            container.avg_frame_rate_num = demux.stream->avg_frame_rate_num;
            container.avg_frame_rate_den = demux.stream->avg_frame_rate_den;
            container.r_frame_rate_num = demux.stream->r_frame_rate_num;
            container.r_frame_rate_den = demux.stream->r_frame_rate_den;
        }
        container.packets.reserve(demux.packets.size());
        for (const auto& packet : demux.packets) {
            container.packets.push_back(streamview::analysis::ContainerPacketTiming{
                .index = packet.index,
                .pts = packet.pts,
                .dts = packet.dts,
                .duration = packet.duration,
                .position = packet.position,
                .keyframe = packet.keyframe,
            });
        }
        return InputData{
            .bytes = std::move(demux.annex_b),
            .codec_hint = demux.codec == streamview::demux::DemuxCodec::H265 ? CodecOverride::H265
                                                                            : CodecOverride::H264,
            .container = std::move(container),
        };
#else
        std::cerr << "streamview: container input requires FFmpeg development libraries at build time\n";
        return std::nullopt;
#endif
    }

    const auto data = read_file(path);
    if (!data.has_value()) {
        return std::nullopt;
    }
    return InputData{.bytes = std::move(*data), .codec_hint = std::nullopt, .container = std::nullopt};
}

void attach_container_timing(streamview::analysis::StreamAnalysis& analysis) {
    if (!analysis.container.has_value()) {
        return;
    }
    const auto& container = *analysis.container;
    if (container.packets.size() != analysis.frames.size()) {
        return;
    }

    for (std::size_t i = 0; i < analysis.frames.size(); ++i) {
        auto& frame = analysis.frames[i];
        const auto& packet = container.packets[i];
        frame.container_packet_index = packet.index;
        frame.pts = packet.pts;
        frame.dts = packet.dts;
        frame.duration = packet.duration;
        frame.packet_position = packet.position;
    }
}

std::optional<streamview::analysis::StreamAnalysis> analyze_input(const std::string& input_path,
                                                                  CodecOverride codec = CodecOverride::Auto) {
    const auto input = load_input_data(input_path);
    if (!input.has_value()) {
        std::cerr << "streamview: failed to read input file: " << input_path << "\n";
        return std::nullopt;
    }

    const auto resolved_codec = codec == CodecOverride::Auto && input->codec_hint.has_value() ? *input->codec_hint
                                                                                              : codec;
    if (resolved_codec == CodecOverride::H265) {
        auto analysis = streamview::analysis::analyze_h265_annex_b(input_path, input->bytes);
        analysis.container = input->container;
        attach_container_timing(analysis);
        streamview::analysis::build_timeline(analysis);
        return analysis;
    }
    auto analysis = streamview::analysis::analyze_h264_annex_b(input_path, input->bytes);
    analysis.container = input->container;
    attach_container_timing(analysis);
    streamview::analysis::build_timeline(analysis);
    return analysis;
}

void write_text_summary(std::ostream& out, const streamview::analysis::StreamAnalysis& analysis) {
    out << "StreamView analysis summary\n";
    out << "Input: " << analysis.input_path << "\n";
    out << "Format: " << analysis.format << "\n";
    out << "Codec: " << analysis.codec_guess << "\n";
    out << "Size bytes: " << analysis.size_bytes << "\n";
    if (analysis.container.has_value()) {
        out << "Container: " << analysis.container->container_format_name;
        if (analysis.container->container_format_long_name.has_value()) {
            out << " (" << *analysis.container->container_format_long_name << ")";
        }
        out << "\n";
        out << "Stream index: " << analysis.container->stream_index << "\n";
        out << "Stream codec: " << analysis.container->stream_codec_name;
        if (analysis.container->stream_codec_long_name.has_value()) {
            out << " (" << *analysis.container->stream_codec_long_name << ")";
        }
        out << "\n";
        out << "Time base: " << analysis.container->time_base_num << "/" << analysis.container->time_base_den
            << "\n";
        if (analysis.container->duration_ts.has_value()) {
            out << "Duration ts: " << *analysis.container->duration_ts << "\n";
        }
        if (analysis.container->start_time_ts.has_value()) {
            out << "Start time ts: " << *analysis.container->start_time_ts << "\n";
        }
        out << "Frame rate: " << analysis.container->avg_frame_rate_num << "/"
            << analysis.container->avg_frame_rate_den << " (r=" << analysis.container->r_frame_rate_num << "/"
            << analysis.container->r_frame_rate_den << ")\n";
        out << "Resolution (container): " << analysis.container->width << "x" << analysis.container->height
            << "\n";
        out << "Bit rate (container): " << analysis.container->bit_rate << "\n";
        out << "Packet count: " << analysis.container->packets.size() << "\n";
        if (!analysis.container->packets.empty()) {
            const auto& packet = analysis.container->packets.front();
            out << "First packet: pts=";
            if (packet.pts.has_value()) {
                out << *packet.pts;
            } else {
                out << "null";
            }
            out << ", dts=";
            if (packet.dts.has_value()) {
                out << *packet.dts;
            } else {
                out << "null";
            }
            out << ", duration=";
            if (packet.duration.has_value()) {
                out << *packet.duration;
            } else {
                out << "null";
            }
            out << ", keyframe=" << (packet.keyframe ? "true" : "false") << "\n";
        }
    }
    if (!analysis.timeline.empty()) {
        const auto& first_timeline = analysis.timeline.front();
        out << "Timeline entries: " << analysis.timeline.size() << "\n";
        out << "First timeline frame: index=" << first_timeline.frame_index
            << ", pts=";
        if (first_timeline.pts.has_value()) {
            out << *first_timeline.pts;
        } else {
            out << "null";
        }
        out << ", dts=";
        if (first_timeline.dts.has_value()) {
            out << *first_timeline.dts;
        } else {
            out << "null";
        }
        out << ", type=" << first_timeline.frame_type << "\n";
    }
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
    if (options.output_format == OutputFormat::Csv) {
        streamview::exporter::write_analysis_csv(*output, analysis);
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
    out << "    \"gop_index\": ";
    if (frame.gop_index.has_value()) {
        out << *frame.gop_index;
    } else {
        out << "null";
    }
    out << ",\n";
    out << "    \"container_packet_index\": ";
    if (frame.container_packet_index.has_value()) {
        out << *frame.container_packet_index;
    } else {
        out << "null";
    }
    out << ",\n";
    out << "    \"pts\": ";
    if (frame.pts.has_value()) {
        out << *frame.pts;
    } else {
        out << "null";
    }
    out << ",\n";
    out << "    \"dts\": ";
    if (frame.dts.has_value()) {
        out << *frame.dts;
    } else {
        out << "null";
    }
    out << ",\n";
    out << "    \"duration\": ";
    if (frame.duration.has_value()) {
        out << *frame.duration;
    } else {
        out << "null";
    }
    out << ",\n";
    out << "    \"packet_position\": ";
    if (frame.packet_position.has_value()) {
        out << *frame.packet_position;
    } else {
        out << "null";
    }
    out << ",\n";
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
    const auto input = load_input_data(options.input_path);
    if (!input.has_value()) {
        std::cerr << "streamview: failed to read input file: " << options.input_path << "\n";
        return 2;
    }

    const auto nals = streamview::bitstream::scan_annex_b(input->bytes);
    if (options.nal_index >= nals.size()) {
        std::cerr << "streamview: NAL index out of range: " << options.nal_index
                  << " >= " << nals.size() << "\n";
        return 2;
    }

    const auto& nal = nals[options.nal_index];
    const auto payload = std::span<const std::uint8_t>(input->bytes).subspan(nal.payload_offset, nal.payload_size);
    if (options.format == DumpFormat::Rbsp) {
        const auto rbsp = streamview::bitstream::nal_payload_to_rbsp(payload);
        return write_dump_output(options, rbsp);
    }

    return write_dump_output(options, payload);
}

ParseDecodeArgsResult parse_decode_args(int argc, char** argv) {
    if (argc < 3) {
        return {.error = "decode requires an input path"};
    }

    DecodeOptions options{.input_path = argv[2]};
    bool frame_set = false;
    for (int i = 3; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--frame") {
            if (i + 1 >= argc) {
                return {.error = "--frame requires a non-negative integer"};
            }
            const auto index = parse_size_arg(argv[++i]);
            if (!index.has_value()) {
                return {.error = "--frame requires a non-negative integer"};
            }
            options.frame_index = *index;
            frame_set = true;
        } else if (arg == "--frames") {
            // Batch range: <start>:<count>
            if (i + 1 >= argc) {
                return {.error = "--frames requires <start>:<count>"};
            }
            const std::string spec = argv[++i];
            const auto colon = spec.find(':');
            if (colon == std::string::npos) {
                return {.error = "--frames requires <start>:<count>"};
            }
            const auto start = parse_size_arg(spec.substr(0, colon));
            const auto count = parse_size_arg(spec.substr(colon + 1));
            if (!start.has_value() || !count.has_value()) {
                return {.error = "--frames requires <start>:<count> non-negative integers"};
            }
            options.frames_start = *start;
            options.frames_count = *count;
            frame_set = true;
        } else if (arg == "--thumb-dir") {
            if (i + 1 >= argc) {
                return {.error = "--thumb-dir requires a directory path"};
            }
            options.thumb_dir = argv[++i];
        } else if (arg == "--thumb") {
            if (i + 1 >= argc) {
                return {.error = "--thumb requires an output path"};
            }
            options.thumbnail_path = argv[++i];
        } else if (arg == "--thumb-size") {
            if (i + 1 >= argc) {
                return {.error = "--thumb-size requires a non-negative integer"};
            }
            const auto size = parse_size_arg(argv[++i]);
            if (!size.has_value()) {
                return {.error = "--thumb-size requires a non-negative integer"};
            }
            options.thumbnail_max_dim = static_cast<int>(*size);
        } else if (arg == "--mv-json") {
            if (i + 1 >= argc) {
                return {.error = "--mv-json requires an output path"};
            }
            options.mv_json_path = argv[++i];
        } else if (arg == "--block-layer") {
            if (i + 1 >= argc) {
                return {.error = "--block-layer requires qp, partition, intra, or motion"};
            }
            const std::string_view value = argv[++i];
            if (value != "qp" && value != "partition" && value != "intra" && value != "motion") {
                return {.error = "--block-layer must be qp, partition, intra, or motion"};
            }
            options.block_layer = std::string(value);
        } else if (arg == "--block-out") {
            if (i + 1 >= argc) {
                return {.error = "--block-out requires an output path"};
            }
            options.block_out = argv[++i];
        } else {
            return {.error = "unknown decode option: " + std::string(arg)};
        }
    }

    if (!frame_set) {
        return {.error = "decode requires --frame <index>"};
    }
    return {.options = std::move(options)};
}

#if defined(STREAMVIEW_HAS_FFMPEG_DECODE)
bool write_ppm(const std::string& path, const streamview::decode::RgbImage& image) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        return false;
    }
    out << "P6\n" << image.width << " " << image.height << "\n255\n";
    out.write(reinterpret_cast<const char*>(image.rgb.data()),
              static_cast<std::streamsize>(image.rgb.size()));
    return static_cast<bool>(out);
}

bool write_mv_json(const std::string& path, const streamview::decode::DecodedFrame& frame) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        return false;
    }
    out << "{\n";
    out << "  \"decode_index\": " << frame.decode_index << ",\n";
    out << "  \"coded_width\": " << frame.coded_width << ",\n";
    out << "  \"coded_height\": " << frame.coded_height << ",\n";
    out << "  \"pict_type\": \"" << frame.pict_type << "\",\n";
    out << "  \"keyframe\": " << (frame.keyframe ? "true" : "false") << ",\n";
    out << "  \"motion_vector_count\": " << frame.motion_vectors.size() << ",\n";
    out << "  \"motion_vectors\": [";
    for (std::size_t i = 0; i < frame.motion_vectors.size(); ++i) {
        const auto& mv = frame.motion_vectors[i];
        out << (i == 0 ? "\n" : ",\n");
        out << "    {\"source\": " << mv.source << ", \"w\": " << mv.w << ", \"h\": " << mv.h
            << ", \"src_x\": " << mv.src_x << ", \"src_y\": " << mv.src_y
            << ", \"dst_x\": " << mv.dst_x << ", \"dst_y\": " << mv.dst_y
            << ", \"motion_x\": " << mv.motion_x << ", \"motion_y\": " << mv.motion_y
            << ", \"motion_scale\": " << mv.motion_scale << "}";
    }
    out << (frame.motion_vectors.empty() ? "]\n" : "\n  ]\n");
    out << "}\n";
    return static_cast<bool>(out);
}

// Batch: decode a contiguous decode-order range in one pass, writing each
// thumbnail as <thumb_dir>/<decode_index>.ppm and printing a JSON index.
int run_decode_range(const DecodeOptions& options) {
    streamview::decode::DecodeRangeOptions range_options{
        .start_index = *options.frames_start,
        .count = options.frames_count,
        .thumbnail_max_dim = options.thumbnail_max_dim,
    };
    const auto result = streamview::decode::decode_frames(options.input_path, range_options);
    if (!result.status.is_ok()) {
        std::cerr << "streamview: decode failed: " << result.status.message() << "\n";
        return 2;
    }
    std::cout << "{\n  \"frames\": [";
    bool first = true;
    for (const auto& frame : result.frames) {
        const std::string ppm =
            *options.thumb_dir + "/" + std::to_string(frame.decode_index) + ".ppm";
        const bool wrote = frame.thumbnail.has_value() && write_ppm(ppm, *frame.thumbnail);
        std::cout << (first ? "\n" : ",\n");
        first = false;
        std::cout << "    {\"decode_index\": " << frame.decode_index
                  << ", \"coded_width\": " << frame.coded_width
                  << ", \"coded_height\": " << frame.coded_height
                  << ", \"pict_type\": \"" << frame.pict_type << "\""
                  << ", \"keyframe\": " << (frame.keyframe ? "true" : "false")
                  << ", \"thumb\": " << (wrote ? ("\"" + ppm + "\"") : "null") << "}";
    }
    std::cout << (result.frames.empty() ? "]\n}\n" : "\n  ]\n}\n");
    return 0;
}

int run_decode(const DecodeOptions& options) {
    if (options.frames_start.has_value() && options.thumb_dir.has_value()) {
        return run_decode_range(options);
    }
    streamview::decode::DecodeOptions decode_options{
        .frame_index = options.frame_index,
        .want_thumbnail = options.thumbnail_path.has_value(),
        .thumbnail_max_dim = options.thumbnail_max_dim,
        .want_motion_vectors = true,
    };
    const auto result = streamview::decode::decode_frame(options.input_path, decode_options);
    if (!result.status.is_ok() || !result.frame.has_value()) {
        std::cerr << "streamview: decode failed: " << result.status.message() << "\n";
        return 2;
    }

    const auto& frame = *result.frame;
    std::cout << "decode_index: " << frame.decode_index << "\n"
              << "resolution: " << frame.coded_width << "x" << frame.coded_height << "\n"
              << "pict_type: " << frame.pict_type << "\n"
              << "keyframe: " << (frame.keyframe ? "true" : "false") << "\n"
              << "motion_vectors: " << frame.motion_vectors.size() << "\n";

    if (options.thumbnail_path.has_value()) {
        if (!frame.thumbnail.has_value()) {
            std::cerr << "streamview: could not build a thumbnail for this frame\n";
            return 2;
        }
        if (!write_ppm(*options.thumbnail_path, *frame.thumbnail)) {
            std::cerr << "streamview: failed to write thumbnail: " << *options.thumbnail_path << "\n";
            return 2;
        }
        std::cout << "thumbnail: " << *options.thumbnail_path << " ("
                  << frame.thumbnail->width << "x" << frame.thumbnail->height << " ppm)\n";
    }

    if (options.mv_json_path.has_value()) {
        if (!write_mv_json(*options.mv_json_path, frame)) {
            std::cerr << "streamview: failed to write MV JSON: " << *options.mv_json_path << "\n";
            return 2;
        }
        std::cout << "mv_json: " << *options.mv_json_path << "\n";
    }

    if (options.block_layer.has_value()) {
        const auto input = load_input_data(options.input_path);
        if (!input.has_value()) {
            std::cerr << "streamview: failed to read input for block overlay\n";
            return 2;
        }
        if (input->codec_hint != CodecOverride::H265) {
            std::cerr << "streamview: block overlays currently support HEVC only\n";
            return 2;
        }
        auto layer = streamview::decode::BlockLayer::Qp;
        const auto& bl = *options.block_layer;
        if (bl == "partition") {
            layer = streamview::decode::BlockLayer::Partition;
        } else if (bl == "intra") {
            layer = streamview::decode::BlockLayer::IntraPred;
        } else if (bl == "motion") {
            layer = streamview::decode::BlockLayer::Motion;
        }
        const auto overlay = streamview::decode::render_hevc_block_overlay(
            input->bytes, options.frame_index, layer, options.thumbnail_max_dim);
        if (!overlay.status.is_ok() || !overlay.image.has_value()) {
            std::cerr << "streamview: block overlay failed: " << overlay.status.message() << "\n";
            return 2;
        }
        const std::string out = options.block_out.value_or("block.ppm");
        if (!write_ppm(out, *overlay.image)) {
            std::cerr << "streamview: failed to write block overlay: " << out << "\n";
            return 2;
        }
        std::cout << "block(" << bl << "): " << out << " (" << overlay.image->width << "x"
                  << overlay.image->height << " ppm)\n";
    }
    return 0;
}
#else
int run_decode(const DecodeOptions&) {
    std::cerr << "streamview: decode is unavailable (built without FFmpeg)\n";
    return 2;
}
#endif

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
    if (command == "decode") {
        const auto result = parse_decode_args(argc, argv);
        if (!result.options.has_value()) {
            std::cerr << "streamview: " << result.error << "\n";
            print_usage(std::cerr);
            return 1;
        }
        return run_decode(*result.options);
    }

    std::cerr << "streamview: unknown command: " << command << "\n";
    print_usage(std::cerr);
    return 1;
}
