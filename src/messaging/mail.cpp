#include "dmn/messaging/mail.hpp"

#include <domino/global.h>
#include <domino/mail.h>
#include <domino/mailserv.h>
#include <domino/ostime.h>

#include <cstring>

#include "dmn/messaging/mime.hpp"
#include "dmn/lmbcs.hpp"
#include "dmn/error.hpp"

using dmn::mail;

mail::mail() : file_hdl_(MailCloseMessageFile), msg_hdl_(MailCloseMessage) {}

auto mail::create(std::optional<std::string_view> mailbox) -> mail {
  // Open the mail message
  mail new_mail = {};
  auto converted = mailbox ? dmn::lmbcs::from_string(*mailbox) : dmn::lmbcs{};
  dmn::status result = MailOpenMessageFile(converted.data(), new_mail.file_hdl_.data());
  result.throw_if_error("Failed to open message file");

  // Create the actual mail
  result = MailCreateMessage(new_mail.file_hdl_.get(), new_mail.msg_hdl_.data());
  result.throw_if_error("Failed to create message");

  return new_mail;
}

void mail::set_body(std::string body, std::string content_type) const {
  dmn::mime::open(*this)
    .set_content_type(std::move(content_type))
    .set_charset("UTF-8")
    .append_content(std::move(body))
    .write_to(*this, MAIL_BODY_ITEM);
}

void mail::add_send_to(std::string_view email) { add_to_list(send_to_, email); }

void mail::add_copy_to(std::string_view email) { add_to_list(copy_to_, email); }

void mail::add_blind_copy_to(std::string_view email) { add_to_list(blind_copy_to_, email); }

void mail::send(std::string_view from, std::string_view subject) {
  // Check if we have anyone to send to
  if (recipients_.empty() || !msg_hdl_) {
    throw dmn::runtime_error("No recipients were added to mail");
  }

  // Add recipients list
  dmn::status result =
    MailAddRecipientsItem(msg_hdl_.get(), recipients_.get_handle(), recipients_.buffer_size());
  result.throw_if_error("Failed to add recipients to mail");

  // The lists below are released because Domino will take ownership
  recipients_.release();

  // Add all lists (SendTo, CopyTo, BlindCopyTo)
  auto add_list = [&](uint8_t item_num, dmn::list& list) {
    if (list.empty()) {
      return;
    }

    result =
      MailAddHeaderItemByHandle(msg_hdl_.get(), item_num, list.get_handle(), list.buffer_size(), 0);
    result.throw_if_error("Failed to add list to mail");
    list.release();
  };

  add_list(MAIL_SENDTO_ITEM_NUM, send_to_);
  add_list(MAIL_COPYTO_ITEM_NUM, copy_to_);
  add_list(MAIL_BLINDCOPYTO_ITEM_NUM, blind_copy_to_);

  // Add mandatory headers
  add_header_item(MAIL_FORM_ITEM_NUM, MAIL_MEMO_FORM, strlen(MAIL_MEMO_FORM));

  add_header_item(MAIL_FROM_ITEM_NUM, from.data(), from.size());
  add_header_item(MAIL_SUBJECT_ITEM_NUM, subject.data(), subject.size());

  TIMEDATE now = {};
  OSCurrentTIMEDATE(&now);
  add_header_item(MAIL_COMPOSEDDATE_ITEM_NUM, &now, sizeof(TIMEDATE));
  add_header_item(MAIL_POSTEDDATE_ITEM_NUM, &now, sizeof(TIMEDATE));

  result = MailTransferMessageLocal(msg_hdl_.get());
  result.throw_if_error("Failed to transfer mail");
}

void mail::add_header_item(uint16_t index, const void* val, size_t size) {
  const dmn::status result = MailAddHeaderItem(msg_hdl_.get(), index, val, size);
  result.throw_if_error("Failed to add header item to mail");
}

void mail::add_to_list(dmn::list& list, std::string_view email) {
  list.push_back(email);
  recipients_.push_back(email);
}
