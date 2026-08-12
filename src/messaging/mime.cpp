#include "dmn/messaging/mime.hpp"

#include <domino/global.h>
#include <domino/nsf.h>
#include <domino/mime.h>

#include <format>
#include <utility>

#include "dmn/error.hpp"

using dmn::mime;

static_assert(sizeof(dmn::mime::handle_t) == sizeof(MIMEHANDLE));

auto mime::set_content_type(std::string content_type) -> mime& {
  content_type_ = std::move(content_type);
  return *this;
}

auto mime::set_charset(std::string charset) -> mime& {
  charset_ = std::move(charset);
  return *this;
}

auto mime::append_content(std::string content) -> mime& {
  content_.push_back(std::move(content));
  return *this;
}

auto mime::open_impl(detail::dhandle_t handle) -> handle_t {
  handle_t mime_hdl = {};
  const dmn::status result = MIMEStreamOpen(handle, nullptr, 0, MIME_STREAM_OPEN_WRITE, &mime_hdl);
  result.throw_if_error("Failed to open MIME stream");

  return mime_hdl;
}

void mime::write_to_impl(detail::dhandle_t handle, std::string field) const {
  write_line(std::format("Content-Type: {}; charset={}", content_type_, charset_));
  write_line("Content-Transfer-Encoding: 8bit");
  write_line("");

  for (const auto& line : content_) {
    write_line(line);
  }

  const dmn::status result =
    MIMEStreamItemize(handle, field.data(), field.size(), MIME_STREAM_ITEMIZE_FULL, hdl_.get());
  result.throw_if_error("Failed to append mime item to note");
}

void mime::write_line(std::string line) const {
  const int error = MIMEStreamPutLine(line.data(), hdl_.get());
  if (error != MIME_STREAM_SUCCESS) {
    throw dmn::mime_error::make("Failed to append line to mime stream", error);
  }
}

mime::mime(handle_t handle) : hdl_(handle, MIMEStreamClose) {};
