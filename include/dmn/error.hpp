#pragma once

#include <exception>
#include <optional>
#include <string>

#include "dmn/detail/lmbcs.hpp"

namespace dmn {
struct status {
  using value_t = uint16_t;
  value_t value;

  constexpr status(value_t code) : value(code) {}

  auto operator==(value_t other) const noexcept -> bool { return value == other; }

  /// Check whether the status code is some form of not-found error.
  ///
  /// \return true if status is a not-found error, false otherwise.
  [[nodiscard]] auto is_not_found() const noexcept -> bool;

  /// Check whether the status code is some form of already-locked error.
  ///
  /// \return true if status is an already-locked error, false otherwise.
  [[nodiscard]] auto is_locked() const noexcept -> bool;

  /// Check whether the status code is an error
  ///
  /// \return true if status is an error, false otherwise.
  [[nodiscard]] auto is_error() const noexcept -> bool;

  /// Throw the status as a `dmn::natve_error` if it is an error.
  ///
  /// \param message Descriptive error message
  /// \throws dmn::native_error If the status is an error.
  void throw_if_error(const char* message) const;
};

constexpr static status no_error{0};

struct error : std::exception {
  explicit error(std::string message) : message_(std::move(message)) {}

  [[nodiscard]] auto what() const noexcept -> const char* override { return message_.c_str(); }

 private:
  std::string message_;
};

struct invalid_handle : error {
  using error::error;
};

struct invalid_argument : error {
  using error::error;
};

struct conversion_error : error {
  using error::error;
};

struct runtime_error : error {
  using error::error;
};

struct out_of_range : error {
  using error::error;
};

struct native_error : error {
  explicit native_error(const char* message, status code);

  /// Get the raw error code.
  [[nodiscard]] auto code() const noexcept -> status { return code_; }

 private:
  status code_;

  /// Loads the error message for a code.
  [[nodiscard]] static auto os_load_string(status code) -> std::optional<lmbcs::str>;

  /// Get the error message for a code.
  [[nodiscard]] static auto get_error_message(std::string message, status code) -> std::string;
};

struct mime_error : error {
  using error::error;

  /// Make a variant of `dmn::mime_error` using its code.
  static auto make(const char* message, int code) -> dmn::mime_error;
};

struct mime_io_error : mime_error {
  using mime_error::mime_error;
};

struct mime_eos_error : mime_error {
  using mime_error::mime_error;
};
}  // namespace dmn