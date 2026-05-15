#include "streamview/bitstream/h265_nal.hpp"

namespace streamview::bitstream {

H265NalHeader parse_h265_nal_header(std::uint8_t first_byte, std::uint8_t second_byte) {
    return {
        .forbidden_zero_bit = static_cast<std::uint8_t>((first_byte >> 7) & 0x01),
        .nal_unit_type = static_cast<H265NalType>((first_byte >> 1) & 0x3F),
        .nuh_layer_id = static_cast<std::uint8_t>(((first_byte & 0x01) << 5) | ((second_byte >> 3) & 0x1F)),
        .nuh_temporal_id_plus1 = static_cast<std::uint8_t>(second_byte & 0x07),
    };
}

bool h265_nal_type_is_vcl(H265NalType type) {
    return static_cast<std::uint8_t>(type) <= 31;
}

std::string_view h265_nal_type_name(H265NalType type) {
    switch (type) {
    case H265NalType::TrailN:
        return "trail_n";
    case H265NalType::TrailR:
        return "trail_r";
    case H265NalType::TsaN:
        return "tsa_n";
    case H265NalType::TsaR:
        return "tsa_r";
    case H265NalType::StsaN:
        return "stsa_n";
    case H265NalType::StsaR:
        return "stsa_r";
    case H265NalType::RadlN:
        return "radl_n";
    case H265NalType::RadlR:
        return "radl_r";
    case H265NalType::RaslN:
        return "rasl_n";
    case H265NalType::RaslR:
        return "rasl_r";
    case H265NalType::BlaWLp:
        return "bla_w_lp";
    case H265NalType::BlaWRadl:
        return "bla_w_radl";
    case H265NalType::BlaNLp:
        return "bla_n_lp";
    case H265NalType::IdrWRadl:
        return "idr_w_radl";
    case H265NalType::IdrNLp:
        return "idr_n_lp";
    case H265NalType::CraNut:
        return "cra_nut";
    case H265NalType::Vps:
        return "vps";
    case H265NalType::Sps:
        return "sps";
    case H265NalType::Pps:
        return "pps";
    case H265NalType::Aud:
        return "aud";
    case H265NalType::PrefixSei:
        return "prefix_sei";
    case H265NalType::SuffixSei:
        return "suffix_sei";
    default:
        return "reserved_or_unspecified";
    }
}

} // namespace streamview::bitstream
