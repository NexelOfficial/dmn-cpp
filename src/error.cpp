#include "dmn/error.hpp"

#include <format>

#include <domino/global.h>
#include <domino/miscerr.h>
#include <domino/oserr.h>
#include <domino/nsferr.h>
#include <domino/osmisc.h>
#include <domino/nsfdata.h>
#include <domino/mime.h>

using dmn::mime_error;
using dmn::native_error;
using dmn::status;

static_assert(sizeof(dmn::status::value) == sizeof(STATUS));
static_assert(dmn::no_error.value == NOERROR);

constexpr uint16_t MAX_MESSAGE_SIZE = 512;

auto status::is_not_found() const noexcept -> bool {
  return value == ERR_NOT_FOUND || value == ERR_NOEXIST || value == ERR_INVALID_NOTE ||
         value == ERR_NOTE_DELETED || value == ERR_ITEM_NOT_FOUND;
}

auto status::is_locked() const noexcept -> bool {
  return value == ERR_NOTE_ALREADY_LOCKED_BY_LOCKER || value == ERR_NOTE_LOCKED ||
         value == ERR_NOTE_LOCKED_BYSOMEONE || value == ERR_NOTE_NOT_LOCKED_BY_LOCKER;
}

auto status::is_error() const noexcept -> bool { return value != dmn::no_error.value; }

void status::throw_if_error(const char* message) const {
  if (is_error()) {
    throw dmn::native_error(message, *this);
  }
}

auto mime_error::make(const char* message, int code) -> dmn::mime_error {
  switch (code) {
    case MIME_STREAM_IO:
      return dmn::mime_io_error{message};
    case MIME_STREAM_EOS:
      return dmn::mime_eos_error{message};
    default:
      return dmn::mime_error{message};
  }
}

native_error::native_error(const char* message, status code)
    : code_(code), error(std::format("{}: {}", message, get_error_message(message, code))) {}

auto native_error::os_load_string(status code) -> std::optional<dmn::lmbcs> {
  dmn::lmbcs buffer;
  buffer.resize(MAX_MESSAGE_SIZE);

  const auto out_size = OSLoadString({}, ERR(code.value), buffer.data(), buffer.size() - 1);
  if (out_size == 0) {
    return std::nullopt;
  }

  buffer.resize(out_size);
  return buffer;
}

auto native_error::get_error_message(std::string message, status code) -> std::string {
  const std::string hex_code = std::format("0x{:x}", ERR(code.value));
  const auto inp = os_load_string(code);
  if (!inp) {
    return std::format("{}: {}", std::move(message), hex_code);
  }

  const auto out = inp->to_string();
  return std::format("{}: {}", std::move(message), out + " (" + hex_code + ")");
}
