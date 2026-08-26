#include "dmn/detail/cursor.hpp"

#include <domino/global.h>
#include <domino/ods.h>

#include "dmn/type.hpp"

using dmn::detail::cursor;

void cursor::write(const void* data, size_t size, dmn::type typ) {
  ensure_bounds(size);
  auto* out = buffer_.subspan(offset_, size).data();
  if (dmn::is_canonical(typ)) {
    ODSWriteMemory(out, static_cast<uint16_t>(typ), data, 1);
  } else {
    memcpy(out, data, size);
  }
  offset_ += size;
}

void cursor::read(void* out, size_t size, dmn::type typ) {
  ensure_bounds(size);
  auto* data = buffer_.subspan(offset_, size).data();
  if (dmn::is_canonical(typ)) {
    ODSReadMemory(data, static_cast<uint16_t>(typ), out, 1);
  } else {
    memcpy(out, data, size);
  }
  offset_ += size;
}