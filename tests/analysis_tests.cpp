#include "streamview/analysis/stream_analysis.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::exit(1);
    }
}

std::vector<std::uint8_t> make_minimal_h264_stream() {
    return {
        0x00, 0x00, 0x00, 0x01,
        0x67, 0x64, 0x00, 0x1f, 0xac, 0xd9, 0x40, 0xa0,
        0x2f, 0xf9, 0x70, 0x11, 0x00, 0x00, 0x03, 0x00,
        0x01, 0x00, 0x00, 0x03, 0x00, 0x32, 0x0f, 0x18,
        0x31, 0x96,
        0x00, 0x00, 0x01,
        0x68, 0xeb, 0xec, 0xb2,
        0x00, 0x00, 0x01,
        0x65, 0x88, 0x80,
    };
}

void test_analyzes_minimal_h264_stream() {
    const auto stream = make_minimal_h264_stream();
    const auto analysis = streamview::analysis::analyze_h264_annex_b("sample.h264", stream);

    require(analysis.input_path == "sample.h264", "analysis input path");
    require(analysis.format == "annex_b", "analysis format");
    require(analysis.codec_guess == "h264", "analysis codec");
    require(analysis.size_bytes == stream.size(), "analysis size");
    require(analysis.nals.size() == 3, "analysis NAL count");
    require(analysis.summary.sps_count == 1, "summary SPS count");
    require(analysis.summary.pps_count == 1, "summary PPS count");
    require(analysis.summary.slices.total == 1, "summary slice count");
    require(analysis.summary.slices.i == 1, "summary I slice count");
    require(analysis.summary.active_sps.has_value(), "summary active SPS");
    require(analysis.summary.active_sps->width == 640, "summary width");
    require(analysis.summary.active_sps->height == 360, "summary height");
    require(analysis.nals[0].h264.sps.has_value(), "first NAL SPS");
    require(analysis.nals[1].h264.pps.has_value(), "second NAL PPS");
    require(analysis.nals[2].h264.slice.has_value(), "third NAL slice");
}

} // namespace

int main() {
    test_analyzes_minimal_h264_stream();
    return 0;
}
