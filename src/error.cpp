#include "dmn/error.hpp"

#include <format>

#include <domino/global.h>
#include <domino/miscerr.h>
#include <domino/oserr.h>
#include <domino/nsferr.h>
#include <domino/osmisc.h>

using dmn::error;
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
    throw dmn::error(message, *this);
  }
}

error::error(const char* message, status code)
    : code_(code), message_(std::format("{}: {}", message, get_error_message(code))) {}

auto error::os_load_string(status code) -> std::optional<lmbcs::str> {
  lmbcs::str buffer(MAX_MESSAGE_SIZE, '\0');
  const uint16_t out_size = OSLoadString(
    WHANDLE{}, ERR(code.value), reinterpret_cast<char*>(buffer.data()), buffer.size() - 1
  );
  if (out_size == 0) {
    return std::nullopt;
  }

  buffer.resize(out_size);
  return buffer;
}

auto error::get_error_message(status code) -> std::string {
  std::string hex_code = std::format("0x{:x}", ERR(code.value));
  const auto inp = os_load_string(code);
  if (!inp) {
    return hex_code;
  }

  const auto out = lmbcs::translate(*inp);
  return out + " (" + hex_code + ")";
}
