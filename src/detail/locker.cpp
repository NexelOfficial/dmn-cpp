#include "dmn/detail/locker.hpp"

#include <domino/global.h>
#include <domino/osmem.h>
#include <domino/pool.h>

#include "dmn/error.hpp"
#include "dmn/addin/session.hpp"
#include "dmn/detail/uhandle.hpp"
#include "dmn/detail/block.hpp"

using dmn::detail::locker;

constexpr static uint32_t MAX_ALLOC_SIZE = 0xFFFFF;

locker::locker(detail::dhandle_t hdl, size_t size, ownership own)
    : locker(detail::block_id{.pool = hdl, .block = 0}, size, own) {}

locker::locker(detail::block_id bid, size_t size, ownership own)
    : detail::cursor(nullptr, size), size_(size), hdl_([own](detail::block_id hdl) {
        if (own != ownership::free) {
          OSUnlock(hdl.pool);
        }
        if (own == ownership::take || own == ownership::free) {
          OSMemFree(hdl.pool);
        }
      }) {
  (void)dmn::session::instance();

  if (bid.pool == detail::dhandle_t{}) {
    return;
  }

  if (own != ownership::free) {
    auto* locked_ptr = OSLockBlock(uint8_t, bid);
    if (locked_ptr == nullptr) {
      return;
    }

    reset(locked_ptr, size);
  }
  hdl_.put(bid);
}

auto locker::allocate_impl(const uint8_t* data, size_t size, ownership own)
  -> std::optional<locker> {
  (void)dmn::session::instance();
  if (size == 0 || size > MAX_ALLOC_SIZE) {
    return std::nullopt;
  }

  detail::dhandle_t out = {};
  const dmn::status result = OSMemAlloc(0, size, &out);
  if (result.is_error()) {
    return std::nullopt;
  }

  {
    auto obj = locker(out, size, ownership::borrow);
    obj.write(data, size);
  }

  return locker(out, size, own);
}
