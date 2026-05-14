#include "streamview/bitstream/annex_b.hpp"

#include <algorithm>

namespace streamview::bitstream {
namespace {

[[nodiscard]] std::size_t start_code_size_at(std::span<const std::uint8_t> data, std::size_t offset) {
    const std::size_t remaining = data.size() - offset;
    if (remaining >= 3 && data[offset] == 0x00 && data[offset + 1] == 0x00 && data[offset + 2] == 0x01) {
        return 3;
    }
    if (remaining >= 4 && data[offset] == 0x00 && data[offset + 1] == 0x00 && data[offset + 2] == 0x00 &&
        data[offset + 3] == 0x01) {
        return 4;
    }
    return 0;
}

[[nodiscard]] std::size_t find_start_code(std::span<const std::uint8_t> data, std::size_t offset) {
    if (data.size() < 3 || offset > data.size() - 3) {
        return data.size();
    }

    for (std::size_t i = offset; i <= data.size() - 3; ++i) {
        if (start_code_size_at(data, i) != 0) {
            return i;
        }
    }
    return data.size();
}

} // namespace

std::vector<NalUnit> scan_annex_b(std::span<const std::uint8_t> data) {
    std::vector<NalUnit> units;

    std::size_t start = find_start_code(data, 0);
    while (start < data.size()) {
        const std::size_t code_size = start_code_size_at(data, start);
        const std::size_t payload = start + code_size;
        const std::size_t next = find_start_code(data, payload);
        const std::size_t payload_end = next == data.size() ? data.size() : next;

        if (payload_end > payload) {
            units.push_back({
                .start_code_offset = start,
                .start_code_size = code_size,
                .payload_offset = payload,
                .payload_size = payload_end - payload,
            });
        }

        start = next;
    }

    return units;
}

} // namespace streamview::bitstream
