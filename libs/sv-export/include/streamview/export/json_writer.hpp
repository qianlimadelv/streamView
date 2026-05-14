#pragma once

#include <ostream>
#include <string_view>

namespace streamview::exporter {

void write_json_string(std::ostream& out, std::string_view value);

} // namespace streamview::exporter
