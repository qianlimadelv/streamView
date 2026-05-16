#include "streamview/analysis/stream_analysis.hpp"

namespace streamview::analysis {
namespace {

void finish_gop(StreamAnalysis& analysis, GopAnalysis& gop, std::size_t end_frame_index) {
    gop.end_frame_index = end_frame_index;
    gop.frame_count = gop.end_frame_index - gop.start_frame_index + 1;
    analysis.gops.push_back(gop);
}

void assign_gop_index_to_frame(StreamAnalysis& analysis, std::size_t frame_index, std::size_t gop_index) {
    if (frame_index >= analysis.frames.size()) {
        return;
    }
    analysis.frames[frame_index].gop_index = gop_index;
}

} // namespace

void build_gops(StreamAnalysis& analysis) {
    analysis.gops.clear();
    analysis.summary.gop_count = 0;

    if (analysis.frames.empty()) {
        return;
    }

    GopAnalysis current{};
    bool has_current = false;

    for (const auto& frame : analysis.frames) {
        if (!has_current) {
            current = {};
            current.index = analysis.gops.size();
            current.start_frame_index = frame.index;
            current.keyframe_index = frame.index;
            current.starts_with_keyframe = frame.is_keyframe;
            has_current = true;
        } else if (frame.is_keyframe) {
            finish_gop(analysis, current, frame.index - 1);
            current = {};
            current.index = analysis.gops.size();
            current.start_frame_index = frame.index;
            current.keyframe_index = frame.index;
            current.starts_with_keyframe = true;
        }

        current.size_bytes += frame.size_bytes;
        assign_gop_index_to_frame(analysis, frame.index, current.index);
    }

    if (has_current) {
        finish_gop(analysis, current, analysis.frames.back().index);
    }

    analysis.summary.gop_count = analysis.gops.size();
}

} // namespace streamview::analysis
