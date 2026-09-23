#pragma once

#include <cstdint>
#include <cstddef>
#include <optional>
#include <string>

#include "dmn/detail/runtime.hpp"
#include "dmn/detail/uhandle.hpp"
#include "dmn/list.hpp"

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
  /// Opens or creates a mail message file and creates a new message within it.
  ///
  /// \param mailbox Mailbox to create the message in. Defaults to "mail.box".
  static auto create(std::optional<std::string_view> mailbox = std::nullopt) -> mail;

  /// Set the message body.
  ///
  /// Writes the body as MIME content using the specified content type. If no content type is
  /// provided, `text/html` is used.
  ///
  /// \param body Message body content.
  /// \param content_type MIME content type to use.
  /// \throws dmn::mime_error If writing the MIME content fails.
  void set_body(std::string body, std::string content_type = "text/html") const;

  /// Add a primary recipient.
  ///
  /// \param email Recipient email address.
  void add_send_to(std::string_view email);

  /// Add a carbon-copy recipient.
  ///
  /// \param email Recipient email address.
  void add_copy_to(std::string_view email);

  /// Add a blind carbon-copy recipient.
  ///
  /// \param email Recipient email address.
  void add_blind_copy_to(std::string_view email);

  /// Send the message.
  ///
  /// Adds recipient and header information, transfers ownership of mail handles to Domino, and
  /// submits the message for delivery.
  ///
  /// \param from Sender email address.
  /// \param subject Message subject.
  /// \throws dmn::runtime_error If no recipients were added to the mail.
  void send(std::string_view from, std::string_view subject);

  [[nodiscard]] auto get_handle() const -> detail::dhandle_t { return msg_hdl_.get(); }

 private:
  detail::uhandle<detail::dhandle_t> file_hdl_;
  detail::uhandle<detail::dhandle_t> msg_hdl_;

  dmn::list send_to_;
  dmn::list copy_to_;
  dmn::list blind_copy_to_;
  dmn::list recipients_;

  /// Internal implementation used by `dmn::mail::send()`.
  void add_header_item(uint16_t index, const void* val, size_t size);

  /// Internal implementation used by add-functions.
  void add_to_list(dmn::list& list, std::string_view email);

  mail();
};
}  // namespace dmn
