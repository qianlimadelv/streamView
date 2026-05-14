#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace streamview::bitstream {

struct NalUnit {
    std::size_t start_code_offset{};
    std::size_t start_code_size{};
    std::size_t payload_offset{};
    std::size_t payload_size{};
};

[[nodiscard]] std::vector<NalUnit> scan_annex_b(std::span<const std::uint8_t> data);

} // namespace streamview::bitstream
