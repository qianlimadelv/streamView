#pragma once

#include "streamview/core/status.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace streamview::demux {

struct H264AnnexBDemuxResult {
    streamview::Status status;
    std::vector<std::uint8_t> annex_b;
};

[[nodiscard]] H264AnnexBDemuxResult demux_h264_to_annex_b(const std::string& input_path);

} // namespace streamview::demux
