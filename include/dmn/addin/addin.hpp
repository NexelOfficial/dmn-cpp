#pragma once

#include <mutex>
#include <sstream>
#include <functional>
#include <string>

#include "dmn/detail/uhandle.hpp"

namespace dmn {
class addin {
 public:
  using function_t = std::function<void()>;

  /// Create and initialize a Domino add-in instance.
  ///
  /// Creates the add-in message queue, configures the add-in status line,
  /// and applies the default add-in settings.
  ///
  /// \param name Name of the add-in.
  /// \return Initialized add-in instance.
  /// \throws dmn::native_error If message queue creation or status line creation fails.
  /// \throws dmn::runtime_error If created status line is invalid.
  static auto create(std::string_view name) -> addin;

  /// Update the add-in status line.
  ///
  /// \param message Status message to display on the server console status line.
  static void update_status(const std::string& message);

  /// Direct wrapper for `AddInMinutesHaveElapsed`.
  static auto minutes_elapsed(size_t minutes) -> bool;

  /// Direct wrapper for `AddInSecondsHaveElapsed`.
  static auto seconds_elapsed(size_t minutes) -> bool;

  /// Direct wrapper for `AddInDayHasElapsed`.
  static auto day_elapsed() -> bool;

  /// Enter the add-in processing loop.
  ///
  /// Repeatedly invokes the supplied callback until the add-in becomes idle,
  /// termination is requested, or an exception is thrown from the callback.
  ///
  /// \param loop_function Function invoked on each loop iteration.
  /// \throws dmn::invalid_handle If the status line handle is invalid.
  /// \note Exceptions thrown by the callback are logged and terminate the loop.
  void enter_loop(const function_t& callback) const;

  /// Log a message to the server console.
  ///
  /// Concatenates the supplied arguments into a single message, and writes the resulting text to
  /// the server log.
  ///
  /// \param args Values to append to the log message.
  template <typename... Args>
  static void log(Args&&... args) noexcept {
    try {
      std::stringstream ss{};
      ss << prefix_.value_or("");
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ((ss << std::forward<Args>(args)), ...);
      log_impl(ss.str());
    } catch (...) {
      return;
    }
  }

  /// Add a custom prefix that will be added before all logs.
  ///
  /// \param prefix Prefix to apply.
  static void set_log_prefix(std::string_view prefix);

 private:
  static std::mutex mtx;
  static inline std::optional<std::string> prefix_;

  detail::uhandle<detail::dhandle_t> status_hdl_;

  /// Internal implementation used by `dmn::addin::log`.
  static void log_impl(const std::string& text);

  addin(detail::dhandle_t handle);
};
}  // namespace dmn