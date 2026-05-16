#pragma once

#include "streamview/analysis/stream_analysis.hpp"

#include <string>
#include <vector>

namespace streamview::analysis {

struct ValidationIssue {
    std::string severity;
    std::string code;
    std::string message;
};

[[nodiscard]] std::vector<ValidationIssue> validate_stream_analysis(const StreamAnalysis& analysis);

} // namespace streamview::analysis
