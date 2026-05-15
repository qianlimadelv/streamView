#pragma once

#include <cstdint>
#include <string_view>

namespace streamview::bitstream {

enum class H265NalType : std::uint8_t {
    TrailN = 0,
    TrailR = 1,
    TsaN = 2,
    TsaR = 3,
    StsaN = 4,
    StsaR = 5,
    RadlN = 6,
    RadlR = 7,
    RaslN = 8,
    RaslR = 9,
    BlaWLp = 16,
    BlaWRadl = 17,
    BlaNLp = 18,
    IdrWRadl = 19,
    IdrNLp = 20,
    CraNut = 21,
    Vps = 32,
    Sps = 33,
    Pps = 34,
    Aud = 35,
    PrefixSei = 39,
    SuffixSei = 40,
};

struct H265NalHeader {
    std::uint8_t forbidden_zero_bit{};
    H265NalType nal_unit_type{H265NalType::TrailN};
    std::uint8_t nuh_layer_id{};
    std::uint8_t nuh_temporal_id_plus1{};
};

[[nodiscard]] H265NalHeader parse_h265_nal_header(std::uint8_t first_byte, std::uint8_t second_byte);
[[nodiscard]] bool h265_nal_type_is_vcl(H265NalType type);
[[nodiscard]] std::string_view h265_nal_type_name(H265NalType type);

} // namespace streamview::bitstream
