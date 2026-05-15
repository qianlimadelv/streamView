#include "streamview/export/analysis_json_writer.hpp"

#include "streamview/bitstream/h264_nal.hpp"
#include "streamview/bitstream/h264_slice.hpp"
#include "streamview/bitstream/h265_nal.hpp"
#include "streamview/export/json_writer.hpp"

namespace streamview::exporter {
namespace {

void write_stream_summary_json(std::ostream& out, const analysis::StreamSummary& summary) {
    out << "  \"stream_summary\": {\n";
    out << "    \"vps_count\": " << summary.vps_count << ",\n";
    out << "    \"sps_count\": " << summary.sps_count << ",\n";
    out << "    \"pps_count\": " << summary.pps_count << ",\n";
    out << "    \"frame_count\": " << summary.frame_count << ",\n";
    out << "    \"keyframe_count\": " << summary.keyframe_count << ",\n";
    out << "    \"slice_count\": " << summary.slices.total << ",\n";
    out << "    \"slice_types\": {\n";
    out << "      \"I\": " << summary.slices.i << ",\n";
    out << "      \"P\": " << summary.slices.p << ",\n";
    out << "      \"B\": " << summary.slices.b << ",\n";
    out << "      \"SP\": " << summary.slices.sp << ",\n";
    out << "      \"SI\": " << summary.slices.si << "\n";
    out << "    }";

    if (summary.active_sps.has_value()) {
        out << ",\n";
        out << "    \"active_sps\": {\n";
        out << "      \"profile_idc\": " << static_cast<int>(summary.active_sps->profile_idc) << ",\n";
        out << "      \"level_idc\": " << static_cast<int>(summary.active_sps->level_idc) << ",\n";
        out << "      \"seq_parameter_set_id\": " << summary.active_sps->seq_parameter_set_id << ",\n";
        out << "      \"width\": " << summary.active_sps->width << ",\n";
        out << "      \"height\": " << summary.active_sps->height << "\n";
        out << "    }\n";
    } else {
        out << "\n";
    }

    out << "  },\n";
}

void write_sps_json(std::ostream& out, const bitstream::H264SpsInfo& sps) {
    out << "        \"sps\": {\n";
    out << "          \"profile_idc\": " << static_cast<int>(sps.profile_idc) << ",\n";
    out << "          \"constraint_flags\": " << static_cast<int>(sps.constraint_flags) << ",\n";
    out << "          \"level_idc\": " << static_cast<int>(sps.level_idc) << ",\n";
    out << "          \"seq_parameter_set_id\": " << sps.seq_parameter_set_id << ",\n";
    out << "          \"chroma_format_idc\": " << sps.chroma_format_idc << ",\n";
    out << "          \"bit_depth_luma\": " << static_cast<int>(sps.bit_depth_luma) << ",\n";
    out << "          \"bit_depth_chroma\": " << static_cast<int>(sps.bit_depth_chroma) << ",\n";
    out << "          \"log2_max_frame_num_minus4\": " << sps.log2_max_frame_num_minus4 << ",\n";
    out << "          \"width\": " << sps.width << ",\n";
    out << "          \"height\": " << sps.height << ",\n";
    out << "          \"frame_mbs_only_flag\": " << (sps.frame_mbs_only_flag ? "true" : "false") << ",\n";
    out << "          \"frame_cropping_flag\": " << (sps.frame_cropping_flag ? "true" : "false") << "\n";
    out << "        }\n";
}

void write_pps_json(std::ostream& out, const bitstream::H264PpsInfo& pps) {
    out << "        \"pps\": {\n";
    out << "          \"pic_parameter_set_id\": " << pps.pic_parameter_set_id << ",\n";
    out << "          \"seq_parameter_set_id\": " << pps.seq_parameter_set_id << ",\n";
    out << "          \"entropy_coding_mode_flag\": " << (pps.entropy_coding_mode_flag ? "true" : "false") << ",\n";
    out << "          \"bottom_field_pic_order_in_frame_present_flag\": "
        << (pps.bottom_field_pic_order_in_frame_present_flag ? "true" : "false") << ",\n";
    out << "          \"num_slice_groups_minus1\": " << pps.num_slice_groups_minus1 << "\n";
    out << "        }\n";
}

void write_slice_json(std::ostream& out, const bitstream::H264SliceHeaderInfo& slice) {
    out << "        \"slice\": {\n";
    out << "          \"first_mb_in_slice\": " << slice.first_mb_in_slice << ",\n";
    out << "          \"slice_type_raw\": " << slice.slice_type_raw << ",\n";
    out << "          \"slice_type\": ";
    write_json_string(out, bitstream::h264_slice_kind_name(slice.slice_kind));
    out << ",\n";
    out << "          \"slice_type_all_slices\": " << (slice.slice_type_all_slices ? "true" : "false") << ",\n";
    out << "          \"pic_parameter_set_id\": " << slice.pic_parameter_set_id << ",\n";
    out << "          \"frame_num\": " << slice.frame_num << "\n";
    out << "        }\n";
}

void write_h264_details_json(std::ostream& out, const analysis::H264NalAnalysis& h264) {
    out << "      \"h264\": {\n";
    out << "        \"forbidden_zero_bit\": " << static_cast<int>(h264.header.forbidden_zero_bit) << ",\n";
    out << "        \"nal_ref_idc\": " << static_cast<int>(h264.header.nal_ref_idc) << ",\n";
    out << "        \"nal_unit_type\": " << static_cast<int>(h264.header.nal_unit_type) << ",\n";
    out << "        \"nal_unit_type_name\": ";
    write_json_string(out, bitstream::h264_nal_type_name(h264.header.nal_unit_type));

    if (h264.sps.has_value()) {
        out << ",\n";
        write_sps_json(out, *h264.sps);
    } else if (h264.pps.has_value()) {
        out << ",\n";
        write_pps_json(out, *h264.pps);
    } else if (h264.slice.has_value()) {
        out << ",\n";
        write_slice_json(out, *h264.slice);
    } else if (h264.sps_parse_error.has_value()) {
        out << ",\n";
        out << "        \"sps_parse_error\": ";
        write_json_string(out, *h264.sps_parse_error);
        out << "\n";
    } else if (h264.pps_parse_error.has_value()) {
        out << ",\n";
        out << "        \"pps_parse_error\": ";
        write_json_string(out, *h264.pps_parse_error);
        out << "\n";
    } else if (h264.slice_parse_error.has_value()) {
        out << ",\n";
        out << "        \"slice_parse_error\": ";
        write_json_string(out, *h264.slice_parse_error);
        out << "\n";
    } else {
        out << "\n";
    }

    out << "      }\n";
}

void write_h265_details_json(std::ostream& out, const analysis::H265NalAnalysis& h265) {
    out << "      \"h265\": {\n";
    out << "        \"forbidden_zero_bit\": " << static_cast<int>(h265.header.forbidden_zero_bit) << ",\n";
    out << "        \"nal_unit_type\": " << static_cast<int>(h265.header.nal_unit_type) << ",\n";
    out << "        \"nal_unit_type_name\": ";
    write_json_string(out, bitstream::h265_nal_type_name(h265.header.nal_unit_type));
    out << ",\n";
    out << "        \"nuh_layer_id\": " << static_cast<int>(h265.header.nuh_layer_id) << ",\n";
    out << "        \"nuh_temporal_id_plus1\": " << static_cast<int>(h265.header.nuh_temporal_id_plus1) << "\n";
    out << "      }\n";
}

void write_frames_json(std::ostream& out, const std::vector<analysis::FrameAnalysis>& frames) {
    out << "  \"frames\": [\n";
    for (std::size_t i = 0; i < frames.size(); ++i) {
        const auto& frame = frames[i];
        out << "    {\n";
        out << "      \"index\": " << frame.index << ",\n";
        out << "      \"codec\": ";
        write_json_string(out, frame.codec);
        out << ",\n";
        out << "      \"frame_type\": ";
        write_json_string(out, frame.frame_type);
        out << ",\n";
        out << "      \"is_keyframe\": " << (frame.is_keyframe ? "true" : "false") << ",\n";
        out << "      \"size_bytes\": " << frame.size_bytes << ",\n";
        out << "      \"first_payload_offset\": " << frame.first_payload_offset << ",\n";
        out << "      \"nal_indices\": [";
        for (std::size_t j = 0; j < frame.nal_indices.size(); ++j) {
            out << frame.nal_indices[j] << (j + 1 == frame.nal_indices.size() ? "" : ", ");
        }
        out << "]\n";
        out << "    }" << (i + 1 == frames.size() ? "\n" : ",\n");
    }
    out << "  ],\n";
}

} // namespace

void write_analysis_json(std::ostream& out, const analysis::StreamAnalysis& analysis) {
    out << "{\n";
    out << "  \"format\": ";
    write_json_string(out, analysis.format);
    out << ",\n";
    out << "  \"codec_guess\": ";
    write_json_string(out, analysis.codec_guess);
    out << ",\n";
    out << "  \"input\": ";
    write_json_string(out, analysis.input_path);
    out << ",\n";
    out << "  \"size_bytes\": " << analysis.size_bytes << ",\n";
    out << "  \"nal_count\": " << analysis.nals.size() << ",\n";
    write_stream_summary_json(out, analysis.summary);
    write_frames_json(out, analysis.frames);
    out << "  \"nals\": [\n";

    for (std::size_t i = 0; i < analysis.nals.size(); ++i) {
        const auto& nal = analysis.nals[i];
        out << "    {\n";
        out << "      \"index\": " << nal.index << ",\n";
        out << "      \"start_code_offset\": " << nal.unit.start_code_offset << ",\n";
        out << "      \"start_code_size\": " << nal.unit.start_code_size << ",\n";
        out << "      \"payload_offset\": " << nal.unit.payload_offset << ",\n";
        out << "      \"payload_size\": " << nal.unit.payload_size << ",\n";
        if (nal.h264.has_value()) {
            write_h264_details_json(out, *nal.h264);
        } else if (nal.h265.has_value()) {
            write_h265_details_json(out, *nal.h265);
        }
        out << "    }" << (i + 1 == analysis.nals.size() ? "\n" : ",\n");
    }

    out << "  ]\n";
    out << "}\n";
}

} // namespace streamview::exporter
