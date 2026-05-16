#pragma once

#include "streamview/analysis/stream_analysis.hpp"

#include <ostream>

namespace streamview::exporter {

void write_analysis_csv(std::ostream& out, const analysis::StreamAnalysis& analysis);

} // namespace streamview::exporter
