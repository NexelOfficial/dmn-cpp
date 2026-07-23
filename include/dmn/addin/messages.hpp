#pragma once

#include <string>
#include <optional>

#include "dmn/os/uhandle.hpp"

namespace dmn {
class addin;

class messages {
 public:
  using handle_t = unsigned long;

  /// Open a named message queue.
  ///
  /// \param name The name of the message queue to open.
  /// \return An instance of `dmn::messages`.
  /// \throws dmn::error If the message queue cannot be opened.
  [[nodiscard]] static auto open(std::string_view name) -> messages;

  /// Retrieve the next message from the queue, if available.
  ///
  /// \return Message contents when a message is available; otherwise an empty result containing the
  /// queue status code.
  /// \throws std::runtime_error If the underlying message queue handle is empty.
  [[nodiscard]] auto get_message() const -> std::optional<std::string>;

 private:
  dmn::uhandle<handle_t> hdl_;

  messages(handle_t handle);
};
}  // namespace dmn