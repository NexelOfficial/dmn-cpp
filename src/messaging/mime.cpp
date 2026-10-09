
#include "dmn/messaging/mime.hpp"

#include <domino/global.h>
#include <domino/nsf.h>
#include <domino/mime.h>

#include <array>
#include <span>
#include <string>
#include <utility>

#include "dmn/detail/scoped_handle.hpp"
#include "dmn/error.hpp"

using dmn::imimestream;
using dmn::mimestream;
using dmn::omimestream;

static_assert(sizeof(dmn::mimestream::handle_t) == sizeof(MIMEHANDLE));

auto mimestream::open_impl(detail::dhandle_t note_handle, const dmn::lmbcs& item, bool is_write)
  -> detail::scoped_handle<handle_t> {
  const auto flags = is_write ? MIME_STREAM_OPEN_WRITE : MIME_STREAM_OPEN_READ;
  handle_t mime_handle = {};
  const dmn::status result =
    MIMEStreamOpen(note_handle, const_cast<char*>(item.c_str()), item.size(), flags, &mime_handle);
  result.throw_if_error("Failed to open MIME stream");
  return detail::scoped_handle<handle_t>(mime_handle, MIMEStreamClose);
}

mimestream::mimestream(detail::dhandle_t note_handle, dmn::lmbcs item, bool is_write)
    : hdl_(open_impl(note_handle, item, is_write)), item_(std::move(item)) {}

auto imimestream::get() -> int {
  if (position_ == size_ && !next_chunk()) {
    return EOF;
  }

  return buffer_.at(position_++);
}

auto imimestream::next_chunk() -> bool {
  if (eof_) {
    return false;
  }

  uint32_t count = 0;
  const int error = MIMEStreamRead(buffer_.data(), &count, buffer_.size(), get_handle());
  if (error != MIME_STREAM_SUCCESS && error != MIME_STREAM_EOS) {
    failed_ = true;
    throw dmn::mime_error("Failed to read from MIME stream");
  }

  position_ = 0;
  size_ = count;
  if (error == MIME_STREAM_EOS || count == 0) {
    eof_ = true;
  }

  return count != 0;
}

auto imimestream::operator>>(std::string& out) -> imimestream& {
  out.clear();
  if (failed_) {
    return *this;
  }

  int ch = get();
  while (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
    ch = get();
  }

  while (ch != EOF && ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n') {
    out.push_back(static_cast<char>(ch));
    ch = get();
  }

  if (ch != EOF) {
    --position_;
  }
  if (out.empty()) {
    failed_ = true;
  }

  return *this;
}

auto imimestream::getline(std::string& out) -> imimestream& {
  out.clear();
  if (failed_) {
    return *this;
  }

  int ch = get();
  while (ch != EOF && ch != '\n') {
    out.push_back(static_cast<char>(ch));
    ch = get();
  }

  if (ch == '\n' && out.ends_with('\r')) {
    out.pop_back();
  }
  if (ch == EOF && out.empty()) {
    failed_ = true;
  }

  return *this;
}

auto imimestream::read(std::span<char> buffer) -> size_t {
  if (failed_) {
    return 0;
  }

  size_t count = 0;
  for (auto& ch : buffer) {
    const int value = get();
    if (value == EOF) {
      break;
    }

    ch = static_cast<char>(value);
    ++count;
  }

  if (count == 0 && !buffer.empty()) {
    failed_ = true;
  }

  return count;
}

auto omimestream::multipart() -> omimestream& {
  if (boundary_) {
    throw dmn::runtime_error("MIME stream is already of type multipart");
  }

  constexpr static uint8_t BOUNDARY_SIZE = std::numeric_limits<uint8_t>::max();
  boundary_ = detail::random_string(BOUNDARY_SIZE);

  const auto value = "multipart/mixed; boundary=\"" + *boundary_ + "\"";
  return *this << dmn::header{{"Content-Type", value}};
}

auto omimestream::boundary() -> omimestream& {
  if (!boundary_) {
    throw dmn::runtime_error("MIME stream is not of type multipart");
  }

  write("\r\n--" + *boundary_ + "\r\n");
  has_content_ = false;
  return *this;
}

auto omimestream::operator<<(header hdr) -> omimestream& {
  if (has_content_) {
    throw dmn::runtime_error("Header is written after content");
  }

  const auto line = std::move(hdr.key) + ": " + std::move(hdr.value) + "\r\n";
  return write(line);
}

auto omimestream::operator<<(std::string_view buffer) -> omimestream& {
  if (!has_content_) {
    write("\r\n");
    has_content_ = true;
  }

  return write(buffer);
}

void omimestream::finalize_impl(detail::dhandle_t note_handle) {
  if (get_handle() == handle_t{}) {
    throw dmn::runtime_error("No active MIME stream to finalize");
  }

  if (boundary_) {
    write("\r\n--" + *boundary_ + "--");
  }

  auto& item = get_item();
  const dmn::status result = MIMEStreamItemize(
    note_handle, item.data(), item.size(), MIME_STREAM_ITEMIZE_FULL, get_handle()
  );
  result.throw_if_error("Failed to finalize mime stream to note item");
}

auto omimestream::write(std::string_view buffer) -> omimestream& {
  const int error = MIMEStreamWrite(
    reinterpret_cast<unsigned char*>(const_cast<char*>(buffer.data())),
    static_cast<unsigned int>(buffer.size()), get_handle()
  );

  if (error != MIME_STREAM_SUCCESS) {
    throw dmn::mime_error("Failed to append data to mime stream");
  }
  return *this;
}
