#pragma once

#include <cstdint>
#include <optional>

#include "dmn/misc/memory.hpp"
#include "dmn/os/uhandle.hpp"

namespace dmn::os {
template <typename T>
concept is_byte_container = std::ranges::contiguous_range<T> && std::ranges::sized_range<T> &&
                            sizeof(std::ranges::range_value_t<T>) == 1;

enum class ownership : uint8_t {
  /// Takes ownership of the handle.
  ///
  /// The handle is locked when acquired and unlocked and freed when the owner is destroyed.
  take,
  /// Borrows the handle without taking ownership.
  ///
  /// The handle is locked when acquired and unlocked when the owner is destroyed.
  borrow,
  /// Takes ownership of the memory behind the handle.
  ///
  /// The handle is freed when the owner is destroyed. Does not lock or unlock the handle.
  free
};

struct block_id {
  dmn::dhandle_t pool;
  uint32_t block;

  auto operator==(const block_id& other) const noexcept -> bool {
    return pool == other.pool && block == other.block;
  }
};

class locker : public dmn::misc::memory {
 public:
  locker() = delete;

  /// Construct a locker from an OS memory handle.
  ///
  /// \param hdl OS memory handle.
  /// \param size Size of the memory region. Defaults to the numerical maximum of size_t.
  /// \param own Ownership mode for the handle. See `dmn::utils::ownership` for details.
  locker(
    dmn::dhandle_t hdl, size_t size = std::numeric_limits<size_t>::max(),
    ownership own = ownership::take
  );

  /// Construct a locker from a block id.
  ///
  /// \param bid Valid block id.
  /// \param size Size of the memory region. Defaults to the numerical maximum of size_t.
  /// \param own Ownership mode for the handle. See `dmn::utils::ownership` for details.
  locker(
    block_id bid, size_t size = std::numeric_limits<size_t>::max(), ownership own = ownership::take
  );

  /// Allocate a chunk of memory on the server
  ///
  /// Copies the contents of the provided container to a new chunk of memory on the server.
  /// \param buffer Byte container with `size()` and `data()` functions.
  /// \param own Ownership mode for the handle. See `dmn::utils::ownership` for details.
  /// \return Instance of `dmn::os::locker` with `ownership::take`.
  /// \note The maximum amount of memory that can be allocated is 1.048.575 bytes.
  template <typename T>
    requires is_byte_container<T>
  static auto allocate(const T& buffer, ownership own = ownership::take) -> std::optional<locker> {
    return allocate_impl(buffer.data(), buffer.size(), own);
  }

  [[nodiscard]] auto release() -> dmn::os::block_id { return hdl_.release(); }

  [[nodiscard]] auto get_block_id() const -> dmn::os::block_id { return hdl_.get(); }

  [[nodiscard]] auto get_handle() const -> dmn::dhandle_t { return hdl_.get().pool; }

  [[nodiscard]] auto size() const -> size_t { return size_; }

 private:
  dmn::uhandle<dmn::os::block_id> hdl_;
  size_t size_;

  /// Internal implementation used by `dmn::os::locker::allocate`.
  static auto allocate_impl(const uint8_t* data, size_t size, ownership own)
    -> std::optional<locker>;
};
}  // namespace dmn::os