#pragma once

#include "streamview/bitstream/annex_b.hpp"
#include "streamview/bitstream/h264_nal.hpp"
#include "streamview/bitstream/h264_pps.hpp"
#include "streamview/bitstream/h264_slice.hpp"
#include "streamview/bitstream/h264_sps.hpp"
#include "streamview/bitstream/h265_nal.hpp"
#include "streamview/bitstream/h265_pps.hpp"
#include "streamview/bitstream/h265_slice.hpp"
#include "streamview/bitstream/h265_sps.hpp"
#include "streamview/bitstream/h265_vps.hpp"

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

struct ParseErrorStats {
    std::size_t total{};
    std::size_t vps{};
    std::size_t sps{};
    std::size_t pps{};
    std::size_t slice{};
};

struct StreamSummary {
    std::optional<bitstream::H264SpsInfo> active_sps;
    std::optional<bitstream::H265VpsInfo> active_h265_vps;
    std::optional<bitstream::H265SpsInfo> active_h265_sps;
    std::size_t vps_count{};
    std::size_t sps_count{};
    std::size_t pps_count{};
    std::size_t frame_count{};
    std::size_t keyframe_count{};
    std::size_t gop_count{};
    SliceStats slices;
    ParseErrorStats parse_errors;
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

struct H265NalAnalysis {
    bitstream::H265NalHeader header{};
    std::optional<bitstream::H265VpsInfo> vps;
    std::optional<bitstream::H265SpsInfo> sps;
    std::optional<bitstream::H265PpsInfo> pps;
    std::optional<bitstream::H265SliceHeaderInfo> slice;
    std::optional<std::string> vps_parse_error;
    std::optional<std::string> sps_parse_error;
    std::optional<std::string> pps_parse_error;
    std::optional<std::string> slice_parse_error;
};

struct NalAnalysis {
    std::size_t index{};
    bitstream::NalUnit unit{};
    std::optional<H264NalAnalysis> h264;
    std::optional<H265NalAnalysis> h265;
};

struct FrameAnalysis {
    std::size_t index{};
    std::size_t decode_order_index{};
    std::optional<std::size_t> gop_index;
    std::string codec;
    std::string frame_type;
    bool is_keyframe{};
    std::optional<std::int64_t> poc;
    std::vector<std::size_t> nal_indices;
    std::size_t size_bytes{};
    std::size_t first_payload_offset{};
};

struct GopAnalysis {
    std::size_t index{};
    std::size_t start_frame_index{};
    std::size_t end_frame_index{};
    std::size_t frame_count{};
    std::size_t keyframe_index{};
    std::size_t size_bytes{};
    bool starts_with_keyframe{};
};

struct StreamAnalysis {
    std::string input_path;
    std::string format{"annex_b"};
    std::string codec_guess{"h264"};
    std::size_t size_bytes{};
    StreamSummary summary;
    std::vector<NalAnalysis> nals;
    std::vector<FrameAnalysis> frames;
    std::vector<GopAnalysis> gops;
};

[[nodiscard]] StreamAnalysis analyze_h264_annex_b(std::string input_path, std::span<const std::uint8_t> data);
[[nodiscard]] StreamAnalysis analyze_h265_annex_b(std::string input_path, std::span<const std::uint8_t> data);
void build_gops(StreamAnalysis& analysis);

} // namespace streamview::analysis
