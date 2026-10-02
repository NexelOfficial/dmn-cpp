#pragma once

#include <concepts>

#include "dmn/detail/scoped_handle.hpp"
#include "dmn/detail/runtime.hpp"
#include "dmn/detail/hook.hpp"

namespace dmn::detail {
template <typename T>
concept block_id_like = requires(T value) {
  { value.pool } -> std::convertible_to<detail::dhandle_t>;
  { value.block } -> std::convertible_to<uint16_t>;
};

class block_id : protected detail::runtime {
 public:
  explicit block_id() = default;

  template <typename T>
    requires block_id_like<T>
  explicit block_id(T bid)
      : pool_(bid.pool), block_(bid.block), generation_(detail::hook::get_generation(bid.pool)){};

  auto operator==(const block_id& other) const noexcept -> bool {
    return pool_ == other.pool_ && block_ == other.block_ && generation_ == other.generation_;
  }

  /// Check if the handle is valid
  [[nodiscard]] auto valid() const -> bool { return detail::hook::is_valid(pool_, generation_); }

  /// Unchecked access to the block handle.
  [[nodiscard]] auto operator*() const -> detail::dhandle_t { return pool_; }

  /// Checked access to the block handle.
  [[nodiscard]] auto handle() const -> detail::dhandle_t {
    if (!valid()) {
      throw dmn::invalid_handle("Block id is empty or invalid");
    }
    return pool_;
  }

  /// Convert checked value to BLOCKID-like type.
  template <typename T>
    requires block_id_like<T>
  [[nodiscard]] auto convert() const -> T {
    if (!valid()) {
      throw dmn::invalid_handle("Block id is empty or invalid");
    }
    return {pool_, block_};
  }

  [[nodiscard]] auto offset() const -> uint16_t { return block_; }

 private:
  detail::dhandle_t pool_{};
  uint16_t block_ = 0;
  uint32_t generation_ = 0;
};
}  // namespace dmn::detail
