#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace streamview::bitstream {

[[nodiscard]] std::vector<std::uint8_t> nal_payload_to_rbsp(std::span<const std::uint8_t> payload);

} // namespace streamview::bitstream
