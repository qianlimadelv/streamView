#include "streamview/analysis/stream_analysis.hpp"
#include "streamview/export/analysis_json_writer.hpp"
#if defined(STREAMVIEW_HAS_FFMPEG_DEMUX)
#include "streamview/demux/ffmpeg_h264_demuxer.hpp"
#endif

#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <filesystem>
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

bool has_extension(const std::string& path, std::string_view extension) {
    return std::filesystem::path(path).extension() == extension;
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
        streamview::exporter::write_analysis_json(output, analysis);
        return 0;
    }

    write_text_summary(std::cout, analysis);
    return 0;
}

int run_analyze(const AnalyzeOptions& options) {
    const auto data = load_input_as_annex_b(options.input_path);
    if (!data.has_value()) {
        std::cerr << "streamview: failed to read input file: " << options.input_path << "\n";
        return 2;
    }

    const auto analysis = (has_extension(options.input_path, ".h265") || has_extension(options.input_path, ".265"))
                              ? streamview::analysis::analyze_h265_annex_b(options.input_path, *data)
                              : streamview::analysis::analyze_h264_annex_b(options.input_path, *data);
    return write_json_output(options, analysis);
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
