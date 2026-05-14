#include "streamview/core/status.hpp"

#include <utility>

namespace streamview {

Status::Status(ErrorCode code, std::string message)
    : code_(code), message_(std::move(message)) {}

Status Status::ok() {
    return {};
}

Status Status::invalid_argument(std::string message) {
    return {ErrorCode::InvalidArgument, std::move(message)};
}

Status Status::io_error(std::string message) {
    return {ErrorCode::IoError, std::move(message)};
}

Status Status::parse_error(std::string message) {
    return {ErrorCode::ParseError, std::move(message)};
}

Status Status::unsupported(std::string message) {
    return {ErrorCode::Unsupported, std::move(message)};
}

bool Status::is_ok() const {
    return code_ == ErrorCode::Ok;
}

ErrorCode Status::code() const {
    return code_;
}

const std::string& Status::message() const {
    return message_;
}

} // namespace streamview
