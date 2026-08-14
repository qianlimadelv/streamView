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
#include "streamview/bitstream/sei.hpp"

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
    std::vector<bitstream::SeiMessage> sei_messages;
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
    std::vector<bitstream::SeiMessage> sei_messages;
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
    std::optional<std::size_t> container_packet_index;
    std::optional<std::int64_t> pts;
    std::optional<std::int64_t> dts;
    std::optional<std::int64_t> duration;
    std::optional<std::int64_t> packet_position;
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

struct ContainerPacketTiming {
    std::size_t index{};
    std::optional<std::int64_t> pts;
    std::optional<std::int64_t> dts;
    std::optional<std::int64_t> duration;
    std::optional<std::int64_t> position;
    bool keyframe{};
};

struct ContainerMetadata {
    std::string container_format_name;
    std::optional<std::string> container_format_long_name;
    std::string stream_codec_name;
    std::optional<std::string> stream_codec_long_name;
    std::size_t stream_index{};
    int time_base_num{};
    int time_base_den{};
    std::optional<std::int64_t> duration_ts;
    std::optional<std::int64_t> start_time_ts;
    int width{};
    int height{};
    std::int64_t bit_rate{};
    int avg_frame_rate_num{};
    int avg_frame_rate_den{};
    int r_frame_rate_num{};
    int r_frame_rate_den{};
    std::vector<ContainerPacketTiming> packets;
};

struct TimelineEntry {
    std::size_t timeline_index{};
    std::size_t frame_index{};
    std::size_t decode_order_index{};
    std::optional<std::size_t> gop_index;
    std::optional<std::size_t> container_packet_index;
    std::optional<std::int64_t> pts;
    std::optional<std::int64_t> dts;
    std::optional<std::int64_t> duration;
    std::optional<std::int64_t> packet_position;
    std::string codec;
    std::string frame_type;
    bool is_keyframe{};
};

struct StreamAnalysis {
    std::string input_path;
    std::string format{"annex_b"};
    std::string codec_guess{"h264"};
    std::size_t size_bytes{};
    std::optional<ContainerMetadata> container;
    StreamSummary summary;
    std::vector<NalAnalysis> nals;
    std::vector<FrameAnalysis> frames;
    std::vector<TimelineEntry> timeline;
    std::vector<GopAnalysis> gops;
};

[[nodiscard]] StreamAnalysis analyze_h264_annex_b(std::string input_path, std::span<const std::uint8_t> data);
[[nodiscard]] StreamAnalysis analyze_h265_annex_b(std::string input_path, std::span<const std::uint8_t> data);
void build_gops(StreamAnalysis& analysis);
void build_timeline(StreamAnalysis& analysis);

} // namespace streamview::analysis
