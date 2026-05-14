#include "streamview/bitstream/rbsp.hpp"

namespace streamview::bitstream {

std::vector<std::uint8_t> nal_payload_to_rbsp(std::span<const std::uint8_t> payload) {
    std::vector<std::uint8_t> rbsp;
    rbsp.reserve(payload.size());

    int zero_count = 0;
    for (const auto byte : payload) {
        if (zero_count >= 2 && byte == 0x03) {
            zero_count = 0;
            continue;
        }

        rbsp.push_back(byte);
        if (byte == 0x00) {
            ++zero_count;
        } else {
            zero_count = 0;
        }
    }

    return rbsp;
}

} // namespace streamview::bitstream
