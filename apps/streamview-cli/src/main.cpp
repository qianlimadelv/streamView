#include "streamview/analysis/stream_analysis.hpp"
#include "streamview/export/analysis_json_writer.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
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

    streamview::exporter::write_analysis_json(std::cout, analysis);
    return 0;
}

int run_analyze(const AnalyzeOptions& options) {
    const auto data = read_file(options.input_path);
    if (!data.has_value()) {
        std::cerr << "streamview: failed to read input file: " << options.input_path << "\n";
        return 2;
    }

    const auto analysis = streamview::analysis::analyze_h264_annex_b(options.input_path, *data);
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
