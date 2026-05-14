#include "streamview/bitstream/bit_reader.hpp"

#include <limits>

namespace streamview::bitstream {

BitReader::BitReader(std::span<const std::uint8_t> data) : data_(data) {}

std::optional<std::uint32_t> BitReader::read_bits(std::size_t count) {
    if (count > 32 || count > bits_remaining()) {
        return std::nullopt;
    }

    std::uint32_t value = 0;
    for (std::size_t i = 0; i < count; ++i) {
        const std::size_t absolute_bit = bit_offset_++;
        const std::uint8_t byte = data_[absolute_bit / 8];
        const std::size_t bit_in_byte = 7 - (absolute_bit % 8);
        value = (value << 1) | ((byte >> bit_in_byte) & 0x01);
    }

    return value;
}

std::optional<bool> BitReader::read_bit() {
    const auto bit = read_bits(1);
    if (!bit.has_value()) {
        return std::nullopt;
    }
    return *bit != 0;
}

std::optional<std::uint32_t> BitReader::read_ue() {
    std::size_t leading_zero_bits = 0;
    while (true) {
        const auto bit = read_bit();
        if (!bit.has_value()) {
            return std::nullopt;
        }
        if (*bit) {
            break;
        }
        ++leading_zero_bits;
        if (leading_zero_bits >= 32) {
            return std::nullopt;
        }
    }

    if (leading_zero_bits == 0) {
        return 0;
    }

    const auto suffix = read_bits(leading_zero_bits);
    if (!suffix.has_value()) {
        return std::nullopt;
    }

    return (static_cast<std::uint32_t>(1U << leading_zero_bits) - 1U) + *suffix;
}

std::optional<std::int32_t> BitReader::read_se() {
    const auto code_num = read_ue();
    if (!code_num.has_value()) {
        return std::nullopt;
    }

    const auto value = static_cast<std::int32_t>((*code_num + 1U) / 2U);
    return (*code_num % 2U) == 0U ? -value : value;
}

std::size_t BitReader::bits_remaining() const {
    return (data_.size() * 8) - bit_offset_;
}

std::size_t BitReader::bit_offset() const {
    return bit_offset_;
}

} // namespace streamview::bitstream
