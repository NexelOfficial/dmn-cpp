#pragma once

#include <cstdint>
#include <cstddef>
#include <optional>
#include <string>

#include "dmn/detail/uhandle.hpp"
#include "dmn/list.hpp"

namespace dmn {

class mail {
 public:
  /// Create a new mail message.
  ///
  /// Opens or creates a mail message file and creates a new message within it.
  ///
  /// \param mail_name Path or name of the mail message file.
  /// \return Instance of `dmn::main`.
  /// \throws dmn::error If the message file cannot be opened or created.
  static auto create(const std::optional<std::string>& name) -> mail;

  /// Set the message body.
  ///
  /// Writes the body as MIME content using the specified content type. If no content type is
  /// provided, `text/html` is used.
  ///
  /// \param body Message body content.
  /// \param content_type MIME content type to use.
  /// \throws dmn::error If the MIME content cannot be created or written.
  /// \throws std::runtime_error If the underlying handle is empty.
  void set_body(const std::string& body, const std::optional<std::string>& content_type) const;

  /// Add a primary recipient.
  ///
  /// \param email Recipient email address.
  void add_send_to(const std::string& email);

  /// Add a carbon-copy recipient.
  ///
  /// \param email Recipient email address.
  void add_copy_to(const std::string& email);

  /// Add a blind carbon-copy recipient.
  ///
  /// \param email Recipient email address.
  void add_blind_copy_to(const std::string& email);

  /// Send the message.
  ///
  /// Adds recipient and header information, transfers ownership of mail handles to Domino, and
  /// submits the message for delivery.
  ///
  /// \param from Sender email address.
  /// \param subject Message subject.
  /// \throws dmn::error If the sending process failed.
  /// \throws std::runtime_error If no recipients were added to the mail.
  void send(const std::string& from, const std::string& subject);

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
  void add_to_list(dmn::list& list, const std::string& email);

  mail();
};
}  // namespace dmn
