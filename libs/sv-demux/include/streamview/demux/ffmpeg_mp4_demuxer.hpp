#pragma once

#include "streamview/core/status.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace streamview::demux {

enum class DemuxCodec {
    H264,
    H265,
};

struct Mp4PacketTiming {
    std::size_t index{};
    std::optional<std::int64_t> pts;
    std::optional<std::int64_t> dts;
    std::optional<std::int64_t> duration;
    std::optional<std::int64_t> position;
    bool keyframe{};
};

struct Mp4StreamMetadata {
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
};

struct Mp4AnnexBDemuxResult {
    streamview::Status status;
    std::vector<std::uint8_t> annex_b;
    std::optional<DemuxCodec> codec;
    std::optional<Mp4StreamMetadata> stream;
    std::vector<Mp4PacketTiming> packets;
};

[[nodiscard]] Mp4AnnexBDemuxResult demux_mp4_to_annex_b(const std::string& input_path);

} // namespace streamview::demux
