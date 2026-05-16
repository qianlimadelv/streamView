#include "streamview/analysis/validation.hpp"

#include <set>
#include <utility>

namespace streamview::analysis {
namespace {

void add_issue(std::vector<ValidationIssue>& issues, std::string severity, std::string code, std::string message) {
    issues.push_back({
        .severity = std::move(severity),
        .code = std::move(code),
        .message = std::move(message),
    });
}

} // namespace

std::vector<ValidationIssue> validate_stream_analysis(const StreamAnalysis& analysis) {
    std::vector<ValidationIssue> issues;
    std::set<std::uint32_t> h264_sps_ids;
    std::set<std::uint32_t> h264_pps_ids;
    std::set<std::uint32_t> h265_vps_ids;
    std::set<std::uint32_t> h265_sps_ids;
    std::set<std::uint32_t> h265_pps_ids;
    std::optional<std::pair<std::uint32_t, std::uint32_t>> h264_resolution;
    std::optional<std::pair<std::uint32_t, std::uint32_t>> h265_resolution;

    for (const auto& nal : analysis.nals) {
        if (nal.h264.has_value()) {
            if (nal.h264->sps.has_value()) {
                const auto [_, inserted] = h264_sps_ids.insert(nal.h264->sps->seq_parameter_set_id);
                if (!inserted) {
                    add_issue(issues, "warning", "duplicate_h264_sps_id", "H.264 SPS id is defined more than once");
                }
                const auto current_resolution = std::pair<std::uint32_t, std::uint32_t>{
                    nal.h264->sps->width,
                    nal.h264->sps->height,
                };
                if (!h264_resolution.has_value()) {
                    h264_resolution = current_resolution;
                } else if (*h264_resolution != current_resolution) {
                    add_issue(issues,
                              "warning",
                              "h264_resolution_change_detected",
                              "H.264 stream contains SPS entries with different resolutions");
                }
            }
            if (nal.h264->pps.has_value()) {
                const auto [_, inserted] = h264_pps_ids.insert(nal.h264->pps->pic_parameter_set_id);
                if (!inserted) {
                    add_issue(issues, "warning", "duplicate_h264_pps_id", "H.264 PPS id is defined more than once");
                }
            }
        }
        if (nal.h265.has_value()) {
            if (nal.h265->vps.has_value()) {
                const auto [_, inserted] = h265_vps_ids.insert(nal.h265->vps->video_parameter_set_id);
                if (!inserted) {
                    add_issue(issues, "warning", "duplicate_h265_vps_id", "H.265 VPS id is defined more than once");
                }
            }
            if (nal.h265->sps.has_value()) {
                const auto [_, inserted] = h265_sps_ids.insert(nal.h265->sps->seq_parameter_set_id);
                if (!inserted) {
                    add_issue(issues, "warning", "duplicate_h265_sps_id", "H.265 SPS id is defined more than once");
                }
                const auto current_resolution = std::pair<std::uint32_t, std::uint32_t>{
                    nal.h265->sps->width,
                    nal.h265->sps->height,
                };
                if (!h265_resolution.has_value()) {
                    h265_resolution = current_resolution;
                } else if (*h265_resolution != current_resolution) {
                    add_issue(issues,
                              "warning",
                              "h265_resolution_change_detected",
                              "H.265 stream contains SPS entries with different resolutions");
                }
            }
            if (nal.h265->pps.has_value()) {
                const auto [_, inserted] = h265_pps_ids.insert(nal.h265->pps->pic_parameter_set_id);
                if (!inserted) {
                    add_issue(issues, "warning", "duplicate_h265_pps_id", "H.265 PPS id is defined more than once");
                }
            }
        }
    }

    if (analysis.summary.gop_count != analysis.gops.size()) {
        add_issue(issues, "warning", "gop_count_mismatch", "summary GOP count does not match the GOP list");
    }
    for (const auto& frame : analysis.frames) {
        if (!frame.gop_index.has_value()) {
            add_issue(issues, "warning", "frame_missing_gop_index", "frame does not have a GOP index assigned");
            continue;
        }
        if (*frame.gop_index >= analysis.gops.size()) {
            add_issue(issues, "error", "frame_gop_index_out_of_range", "frame GOP index is out of range");
            continue;
        }
        const auto& gop = analysis.gops[*frame.gop_index];
        if (frame.index < gop.start_frame_index || frame.index > gop.end_frame_index) {
            add_issue(issues, "error", "frame_gop_index_mismatch", "frame is not covered by its GOP range");
        }
    }
    for (const auto& gop : analysis.gops) {
        if (gop.start_frame_index > gop.end_frame_index) {
            add_issue(issues,
                      "error",
                      "invalid_gop_range",
                      "GOP start frame index is greater than the end frame index");
        }
        if (gop.keyframe_index < gop.start_frame_index || gop.keyframe_index > gop.end_frame_index) {
            add_issue(issues,
                      "error",
                      "invalid_gop_keyframe_index",
                      "GOP keyframe index is outside the GOP range");
        }
    }

    if (analysis.summary.parse_errors.total > 0) {
        add_issue(issues, "error", "parse_errors", "bitstream contains parser errors");
    }
    if (analysis.nals.empty()) {
        add_issue(issues, "error", "no_nal_units", "input does not contain Annex B NAL units");
        return issues;
    }
    if (analysis.summary.frame_count == 0) {
        add_issue(issues, "warning", "no_frames", "no coded frames were identified");
    }
    if (analysis.summary.keyframe_count == 0) {
        add_issue(issues, "warning", "no_keyframes", "no keyframes were identified");
    }
    if (analysis.codec_guess == "h264" && analysis.summary.sps_count == 0) {
        add_issue(issues, "warning", "missing_h264_sps", "H.264 stream has no parsed SPS");
    }
    if (analysis.codec_guess == "h264" && analysis.summary.pps_count == 0) {
        add_issue(issues, "warning", "missing_h264_pps", "H.264 stream has no parsed PPS");
    }
    for (const auto& nal : analysis.nals) {
        if (!nal.h264.has_value()) {
            continue;
        }
        if (nal.h264->pps.has_value() && h264_sps_ids.find(nal.h264->pps->seq_parameter_set_id) == h264_sps_ids.end()) {
            add_issue(issues,
                      "error",
                      "h264_pps_references_missing_sps",
                      "H.264 PPS references an SPS id that was not parsed");
        }
        if (nal.h264->slice.has_value() &&
            h264_pps_ids.find(nal.h264->slice->pic_parameter_set_id) == h264_pps_ids.end()) {
            add_issue(issues,
                      "error",
                      "h264_slice_references_missing_pps",
                      "H.264 slice references a PPS id that was not parsed");
        }
    }
    if (analysis.codec_guess == "h265" && analysis.summary.vps_count == 0) {
        add_issue(issues, "warning", "missing_h265_vps", "H.265 stream has no parsed VPS");
    }
    if (analysis.codec_guess == "h265" && analysis.summary.sps_count == 0) {
        add_issue(issues, "warning", "missing_h265_sps", "H.265 stream has no parsed SPS");
    }
    if (analysis.codec_guess == "h265" && analysis.summary.pps_count == 0) {
        add_issue(issues, "warning", "missing_h265_pps", "H.265 stream has no parsed PPS");
    }
    for (const auto& nal : analysis.nals) {
        if (!nal.h265.has_value()) {
            continue;
        }
        if (nal.h265->sps.has_value() &&
            h265_vps_ids.find(nal.h265->sps->video_parameter_set_id) == h265_vps_ids.end()) {
            add_issue(issues,
                      "error",
                      "h265_sps_references_missing_vps",
                      "H.265 SPS references a VPS id that was not parsed");
        }
        if (nal.h265->pps.has_value() &&
            h265_sps_ids.find(nal.h265->pps->seq_parameter_set_id) == h265_sps_ids.end()) {
            add_issue(issues,
                      "error",
                      "h265_pps_references_missing_sps",
                      "H.265 PPS references an SPS id that was not parsed");
        }
        if (nal.h265->slice.has_value() &&
            h265_pps_ids.find(nal.h265->slice->slice_pic_parameter_set_id) == h265_pps_ids.end()) {
            add_issue(issues,
                      "error",
                      "h265_slice_references_missing_pps",
                      "H.265 slice references a PPS id that was not parsed");
        }
    }
    return issues;
}

} // namespace streamview::analysis
