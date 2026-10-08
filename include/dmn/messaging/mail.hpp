#pragma once

#include <cstdint>
#include <cstddef>
#include <optional>

#include "dmn/detail/scoped_handle.hpp"
#include "dmn/detail/runtime.hpp"
#include "dmn/list.hpp"
#include "dmn/messaging/mime.hpp"

namespace dmn {
/// Mail message being composed for delivery.
///
/// \throws dmn::invalid_handle If an underlying handle is empty.
/// \throws dmn::native_error In case of a lower level failure.
/// \throws dmn::out_of_range If a recipient list exceeds its capacity.
class mail : protected detail::runtime {
 public:
  /// Create a new mail message.
  ///
  /// \param mailbox Mailbox to create the message in. Defaults to "mail.box".
  explicit mail(std::optional<std::string_view> mailbox = {});

  /// Append data to the message body.
  ///
  /// \throws dmn::mime_error If writing the content fails.
  auto operator<<(std::string_view buffer) -> mail&;

  /// Add a primary recipient.
  ///
  /// \param email Recipient e-mail address.
  void add_send_to(std::string_view email);

  /// Add a carbon-copy recipient.
  ///
  /// \param email Recipient e-mail address.
  void add_copy_to(std::string_view email);

  /// Add a blind carbon-copy recipient.
  ///
  /// \param email Recipient e-mail address.
  void add_blind_copy_to(std::string_view email);

  /// Send the message.
  ///
  /// \param from Sender e-mail address.
  /// \param subject Message subject.
  /// \throws dmn::runtime_error If no recipients were added to the mail.
  void send(std::string_view from, std::string_view subject);

  [[nodiscard]] auto get_handle() const -> detail::dhandle_t { return msg_hdl_.get(); }

 private:
  detail::scoped_handle<detail::dhandle_t> file_hdl_;
  detail::scoped_handle<detail::dhandle_t> msg_hdl_;
  std::optional<dmn::omimestream> body_;

  dmn::list send_to_;
  dmn::list copy_to_;
  dmn::list blind_copy_to_;
  dmn::list recipients_;

  /// Internal implementation used by `dmn::mail::send()`.
  void add_header_item(uint16_t index, const void* val, size_t size);

  /// Internal implementation used by add-functions.
  void add_to_list(dmn::list& list, std::string_view email);
};
}  // namespace dmn
