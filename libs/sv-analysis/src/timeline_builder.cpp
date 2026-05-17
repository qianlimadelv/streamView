#include "streamview/analysis/stream_analysis.hpp"

#include <algorithm>
#include <utility>

namespace streamview::analysis {
void build_timeline(StreamAnalysis& analysis) {
    analysis.timeline.clear();
    analysis.timeline.reserve(analysis.frames.size());

    for (const auto& frame : analysis.frames) {
        analysis.timeline.push_back(TimelineEntry{
            .timeline_index = 0,
            .frame_index = frame.index,
            .decode_order_index = frame.decode_order_index,
            .gop_index = frame.gop_index,
            .container_packet_index = frame.container_packet_index,
            .pts = frame.pts,
            .dts = frame.dts,
            .duration = frame.duration,
            .packet_position = frame.packet_position,
            .codec = frame.codec,
            .frame_type = frame.frame_type,
            .is_keyframe = frame.is_keyframe,
        });
    }

    std::stable_sort(analysis.timeline.begin(), analysis.timeline.end(), [](const TimelineEntry& lhs,
                                                                           const TimelineEntry& rhs) {
        const auto lhs_key = lhs.pts.has_value() ? lhs.pts : lhs.dts;
        const auto rhs_key = rhs.pts.has_value() ? rhs.pts : rhs.dts;
        if (lhs_key.has_value() && rhs_key.has_value()) {
            if (*lhs_key != *rhs_key) {
                return *lhs_key < *rhs_key;
            }
        } else if (lhs_key.has_value() != rhs_key.has_value()) {
            return lhs_key.has_value();
        }

        if (lhs.decode_order_index != rhs.decode_order_index) {
            return lhs.decode_order_index < rhs.decode_order_index;
        }
        return lhs.frame_index < rhs.frame_index;
    });

    for (std::size_t i = 0; i < analysis.timeline.size(); ++i) {
        analysis.timeline[i].timeline_index = i;
    }
}

} // namespace streamview::analysis
