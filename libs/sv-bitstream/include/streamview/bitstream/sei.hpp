#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace streamview::bitstream {

// One SEI message: header (payloadType + payloadSize) plus a few decoded fields
// for common payloads. ITU-T H.264 7.3.2.3.1 / H.265 7.3.5.
struct SeiMessage {
    std::uint32_t payload_type{};
    std::uint32_t payload_size{};
    // recovery_point (type 6)
    std::optional<std::int32_t> recovery_frame_cnt;
    // user_data_unregistered (type 5): 16-byte UUID as hex
    std::optional<std::string> user_data_uuid;
};

// Parse the SEI messages in a NAL payload. nal_header_size is the number of NAL
// header bytes to skip (1 for H.264, 2 for H.265).
[[nodiscard]] std::vector<SeiMessage> parse_sei_messages(std::span<const std::uint8_t> nal_payload,
                                                         std::size_t nal_header_size);

[[nodiscard]] std::string_view sei_payload_type_name(std::uint32_t payload_type);

} // namespace streamview::bitstream
