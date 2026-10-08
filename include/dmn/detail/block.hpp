#pragma once

#include "dmn/detail/uhandle.hpp"

namespace dmn::detail {
template <typename T>
concept block_id_like = requires(T value) {
  { value.pool } -> std::convertible_to<detail::dhandle_t>;
  { value.block } -> std::convertible_to<uint16_t>;
};

struct block_id {
  detail::dhandle_t pool;
  uint16_t block;

  explicit block_id() = default;

  template <typename T>
    requires block_id_like<T>
  explicit block_id(T bid) : pool(bid.pool), block(bid.block){};

  template <typename T>
    requires block_id_like<T>
  [[nodiscard]] auto convert() const -> T {
    return {pool, block};
  }

  auto operator==(const block_id& other) const noexcept -> bool {
    return pool == other.pool && block == other.block;
  }
};
}  // namespace dmn::detail
