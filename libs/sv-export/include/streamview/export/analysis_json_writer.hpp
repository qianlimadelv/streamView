#pragma once

#include "streamview/analysis/stream_analysis.hpp"

#include <optional>
#include <ostream>

namespace streamview::exporter {

enum class AnalysisJsonMode {
    Full,
    Summary,
};

struct AnalysisJsonOptions {
    AnalysisJsonMode mode{AnalysisJsonMode::Full};
    std::optional<std::size_t> nal_limit;
};

void write_analysis_json(std::ostream& out, const analysis::StreamAnalysis& analysis);
void write_analysis_json(std::ostream& out, const analysis::StreamAnalysis& analysis, AnalysisJsonMode mode);
void write_analysis_json(std::ostream& out, const analysis::StreamAnalysis& analysis, AnalysisJsonOptions options);

} // namespace streamview::exporter
