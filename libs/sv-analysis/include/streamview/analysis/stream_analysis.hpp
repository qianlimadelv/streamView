#pragma once

#include "streamview/bitstream/annex_b.hpp"
#include "streamview/bitstream/h264_nal.hpp"
#include "streamview/bitstream/h264_pps.hpp"
#include "streamview/bitstream/h264_slice.hpp"
#include "streamview/bitstream/h264_sps.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace streamview::analysis {

struct SliceStats {
    std::size_t total{};
    std::size_t i{};
    std::size_t p{};
    std::size_t b{};
    std::size_t sp{};
    std::size_t si{};
};

struct StreamSummary {
    std::optional<bitstream::H264SpsInfo> active_sps;
    std::size_t sps_count{};
    std::size_t pps_count{};
    SliceStats slices;
};

struct H264NalAnalysis {
    bitstream::H264NalHeader header{};
    std::optional<bitstream::H264SpsInfo> sps;
    std::optional<bitstream::H264PpsInfo> pps;
    std::optional<bitstream::H264SliceHeaderInfo> slice;
    std::optional<std::string> sps_parse_error;
    std::optional<std::string> pps_parse_error;
    std::optional<std::string> slice_parse_error;
};

struct NalAnalysis {
    std::size_t index{};
    bitstream::NalUnit unit{};
    H264NalAnalysis h264;
};

struct StreamAnalysis {
    std::string input_path;
    std::string format{"annex_b"};
    std::string codec_guess{"h264"};
    std::size_t size_bytes{};
    StreamSummary summary;
    std::vector<NalAnalysis> nals;
};

[[nodiscard]] StreamAnalysis analyze_h264_annex_b(std::string input_path, std::span<const std::uint8_t> data);

} // namespace streamview::analysis
