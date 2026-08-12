#pragma once

#include <string>
#include <optional>

#include "dmn/detail/uhandle.hpp"

namespace dmn {
class addin;

class messages {
 public:
#ifdef W32
  using handle_t = unsigned long;
#else
  using handle_t = unsigned int;
#endif

  /// Open a named message queue.
  ///
  /// \param name The name of the message queue to open.
  /// \return An instance of `dmn::messages`.
  /// \throws dmn::native_error If the message queue cannot be opened.
  [[nodiscard]] static auto open(std::string_view name) -> messages;

  /// Retrieve the next message from the queue, if available.
  ///
  /// \return Message contents when a message is available; otherwise an empty result containing the
  /// queue status code.
  /// \throws dmn::invalid_handle If the underlying message queue handle is empty.
  [[nodiscard]] auto get_message() const -> std::optional<std::string>;

 private:
  detail::uhandle<handle_t> hdl_;

  messages(handle_t handle);
};
}  // namespace dmn