#pragma once

#include "streamview/analysis/stream_analysis.hpp"

#include <ostream>

namespace streamview::exporter {

enum class AnalysisJsonMode {
    Full,
    Summary,
};

void write_analysis_json(std::ostream& out, const analysis::StreamAnalysis& analysis);
void write_analysis_json(std::ostream& out, const analysis::StreamAnalysis& analysis, AnalysisJsonMode mode);

} // namespace streamview::exporter
