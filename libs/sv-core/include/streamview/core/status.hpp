#pragma once

#include <string>

namespace streamview {

enum class ErrorCode {
    Ok = 0,
    InvalidArgument,
    IoError,
    ParseError,
    Unsupported,
};

class Status {
public:
    Status() = default;
    Status(ErrorCode code, std::string message);

    [[nodiscard]] static Status ok();
    [[nodiscard]] static Status invalid_argument(std::string message);
    [[nodiscard]] static Status io_error(std::string message);
    [[nodiscard]] static Status parse_error(std::string message);
    [[nodiscard]] static Status unsupported(std::string message);

    [[nodiscard]] bool is_ok() const;
    [[nodiscard]] ErrorCode code() const;
    [[nodiscard]] const std::string& message() const;

private:
    ErrorCode code_{ErrorCode::Ok};
    std::string message_;
};

} // namespace streamview
