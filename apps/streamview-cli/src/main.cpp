#include "streamview/bitstream/annex_b.hpp"
#include "streamview/bitstream/h264_nal.hpp"
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
        out << "\n";
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
