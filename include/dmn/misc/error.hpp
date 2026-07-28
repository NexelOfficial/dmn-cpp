#pragma once

#include <optional>
#include <string>

#include "dmn/os/lmbcs.hpp"

namespace dmn {
struct status {
  using value_t = uint16_t;
  value_t value;

  constexpr status(value_t code) : value(code) {}

  auto operator==(value_t other) const -> bool { return value == other; }

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

  /// Throw the status as a `dmn::error` if it is an error.
  ///
  /// \param message Descriptive error message
  /// \throws dmn::error If the status is an error.
  void throw_if_error(const char* message) const;
};

constexpr static status no_error = status(0);

class error : public std::exception {
 public:
  explicit error(const char* message, status code);

  /// Get the final error message.
  [[nodiscard]] auto what() const noexcept -> const char* override { return message_.c_str(); }

  /// Get the raw error code.
  [[nodiscard]] auto code() const noexcept -> status { return code_; }

 private:
  status code_;
  std::string message_;

  /// Loads the error message for a code.
  ///
  /// \param code Code to load the message for.
  /// \return Error message for \p code , if available.
  [[nodiscard]] static auto os_load_string(status code) -> std::optional<lmbcs::str>;

  /// Get the error message for a code.
  ///
  /// Tries to load the error message for the provided code. If the message for the code could not
  /// be loaded, the code is converted to a hexadecimal string instead.
  ///
  /// \param code Code to get a message for.
  /// \return Error message for \p code if it exists, the code as a hex-string otherwise.
  [[nodiscard]] static auto get_error_message(status code) -> std::string;
};

}  // namespace dmn