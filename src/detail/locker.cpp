#include "dmn/detail/locker.hpp"

#include <domino/global.h>
#include <domino/osmem.h>
#include <domino/pool.h>

#include "dmn/detail/scoped_handle.hpp"
#include "dmn/detail/runtime.hpp"
#include "dmn/detail/block.hpp"
#include "dmn/error.hpp"

using dmn::detail::locker;

constexpr static uint32_t MAX_ALLOC_SIZE = 0xFFFFF;

locker::locker(detail::dhandle_t hdl, ownership own)
    : locker(detail::block_id{BLOCKID(hdl, 0)}, own) {}

locker::locker(detail::block_id bid, ownership own) : locker(bid, 0, own) {
  DWORD names_size = 0;
  const dmn::status result = OSMemGetSize(bid.handle(), &names_size);
  result.throw_if_error("Failed to determine size of memory");
  size_ = names_size;
}

locker::locker(detail::dhandle_t hdl, size_t size, ownership own)
    : locker(detail::block_id{BLOCKID(hdl, 0)}, size, own) {}

locker::locker(detail::block_id bid, size_t size, ownership own)
    : detail::cursor(nullptr, size), size_(size), own_(own), hdl_([own](detail::block_id bid) {
        if (*bid == detail::dhandle_t{}) {
          return;
        }
        OSUnlock(*bid);
        if (own == ownership::take) {
          OSMemFree(*bid);
        }
      }) {
  auto* locked_ptr = OSLockBlock(std::byte, bid.convert<BLOCKID>());
  if (locked_ptr == nullptr) {
    throw dmn::invalid_handle("Memory pointed to by handle is empty");
  }

  reset(locked_ptr, size);
  hdl_.put(bid);
}

auto locker::allocate_impl(size_t size, ownership own) -> locker {
  if (size == 0) {
    throw dmn::invalid_argument("Size cannot be zero");
  }

  detail::session::instance();
  detail::dhandle_t out = {};
  const dmn::status result = OSMemAlloc(0, size, &out);
  result.throw_if_error("Failed to allocate memory");
  return {out, size, own};
}
