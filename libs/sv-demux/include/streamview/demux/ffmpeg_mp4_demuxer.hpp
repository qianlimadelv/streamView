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

struct Mp4AnnexBDemuxResult {
    streamview::Status status;
    std::vector<std::uint8_t> annex_b;
    std::optional<DemuxCodec> codec;
};

[[nodiscard]] Mp4AnnexBDemuxResult demux_mp4_to_annex_b(const std::string& input_path);

} // namespace streamview::demux
