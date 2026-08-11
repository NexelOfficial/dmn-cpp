#pragma once

#include "dmn/detail/uhandle.hpp"

namespace dmn::detail {
struct block_id {
  detail::dhandle_t pool;
  uint32_t block;

  auto operator==(const block_id& other) const noexcept -> bool {
    return pool == other.pool && block == other.block;
  }
};
}  // namespace dmn::detail
