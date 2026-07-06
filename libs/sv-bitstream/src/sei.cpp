#include "streamview/bitstream/sei.hpp"

#include "streamview/bitstream/bit_reader.hpp"
#include "streamview/bitstream/rbsp.hpp"

#include <array>
#include <cstdio>

namespace streamview::bitstream {
namespace {

// Decode the internals of a few common, context-free SEI payloads.
void decode_sei_payload(SeiMessage& msg, std::span<const std::uint8_t> payload) {
    if (msg.payload_type == 5 && payload.size() >= 16) { // user_data_unregistered
        std::array<char, 33> hex{};
        for (std::size_t i = 0; i < 16; ++i) {
            std::snprintf(hex.data() + i * 2, 3, "%02x", payload[i]);
        }
        msg.user_data_uuid = std::string(hex.data(), 32);
    } else if (msg.payload_type == 6) { // recovery_point
        BitReader reader(payload);
        if (const auto cnt = reader.read_ue(); cnt.has_value()) {
            msg.recovery_frame_cnt = static_cast<std::int32_t>(*cnt);
        }
    }
}

} // namespace

std::vector<SeiMessage> parse_sei_messages(std::span<const std::uint8_t> nal_payload,
                                           std::size_t nal_header_size) {
    std::vector<SeiMessage> messages;
    const auto rbsp = nal_payload_to_rbsp(nal_payload);
    if (rbsp.size() <= nal_header_size) {
        return messages;
    }
    const std::span<const std::uint8_t> body =
        std::span<const std::uint8_t>(rbsp).subspan(nal_header_size);

    std::size_t pos = 0;
    // Each sei_message: payloadType and payloadSize are coded as a run of 0xFF
    // bytes plus a final byte. rbsp_trailing_bits (0x80) marks the end.
    while (pos + 1 < body.size()) {
        if (body[pos] == 0x80) {
            break; // rbsp_trailing_bits
        }
        std::uint32_t type = 0;
        while (pos < body.size() && body[pos] == 0xFF) {
            type += 255;
            ++pos;
        }
        if (pos >= body.size()) {
            break;
        }
        type += body[pos++];

        std::uint32_t size = 0;
        while (pos < body.size() && body[pos] == 0xFF) {
            size += 255;
            ++pos;
        }
        if (pos >= body.size()) {
            break;
        }
        size += body[pos++];

        SeiMessage msg{type, size};
        if (size <= body.size() - pos) {
            decode_sei_payload(msg, body.subspan(pos, size));
        }
        messages.push_back(std::move(msg));
        if (size > body.size() - pos) {
            break; // truncated / malformed
        }
        pos += size;
    }
    return messages;
}

std::string_view sei_payload_type_name(std::uint32_t payload_type) {
    switch (payload_type) {
    case 0:
        return "buffering_period";
    case 1:
        return "pic_timing";
    case 2:
        return "pan_scan_rect";
    case 3:
        return "filler_payload";
    case 4:
        return "user_data_registered_itu_t_t35";
    case 5:
        return "user_data_unregistered";
    case 6:
        return "recovery_point";
    case 45:
        return "frame_packing_arrangement";
    case 47:
        return "display_orientation";
    case 129:
        return "active_parameter_sets";
    case 132:
        return "decoded_picture_hash";
    case 137:
        return "mastering_display_colour_volume";
    case 144:
        return "content_light_level";
    default:
        return "reserved_sei_message";
    }
}

} // namespace streamview::bitstream
