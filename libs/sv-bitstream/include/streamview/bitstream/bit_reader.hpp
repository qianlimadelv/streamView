#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace streamview::bitstream {

class BitReader {
public:
    explicit BitReader(std::span<const std::uint8_t> data);

    [[nodiscard]] std::optional<std::uint32_t> read_bits(std::size_t count);
    [[nodiscard]] std::optional<bool> read_bit();
    [[nodiscard]] std::optional<std::uint32_t> read_ue();
    [[nodiscard]] std::optional<std::int32_t> read_se();

    [[nodiscard]] std::size_t bits_remaining() const;
    [[nodiscard]] std::size_t bit_offset() const;

private:
    std::span<const std::uint8_t> data_;
    std::size_t bit_offset_{};
};

} // namespace streamview::bitstream
