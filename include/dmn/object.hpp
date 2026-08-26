#pragma once

#include <cstddef>

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "dmn/detail/object_value.hpp"
#include "dmn/detail/locker.hpp"
#include "dmn/type.hpp"

namespace dmn {
class note;
class view;

class object {
  struct state {
    detail::block_id bid{};
    size_t size{};
  };

 public:
  object() = default;

  /// Check whether the object is empty.
  ///
  /// An object is considered empty when its size is two bytes or less. At that point it's
  /// either completely empty or only has a type but no data.
  [[nodiscard]] auto empty() const noexcept -> bool;

  /// Extract the type of the object.
  [[nodiscard]] auto get_type() const -> dmn::type;

  /// Write data to the memory behind the object.
  ///
  /// \param typ Underlying Domino data type.
  /// \param data Data buffer to write.
  /// \throws dmn::runtime_error If the object is not an item value.
  /// \throws dmn::native_error If the reallocation failed.
  /// \note Writing data is not possible when the object does not belong to an item value.
  /// \note All copies of the object will point to the new memory.
  void write(dmn::type typ, std::span<const std::byte> data);

  /// Check whether the object can be converted to a type.
  template <typename T>
    requires detail::has_object_typecheck<T>
  [[nodiscard]] auto is() const -> bool {
    if (!state_ || state_->size < sizeof(dmn::type) || state_->bid.pool == detail::dhandle_t{}) {
      return false;
    }

    auto cs = get_cursor();
    return detail::object_value<T>::is(cs);
  }

  /// Try to convert the object to a type.
  ///
  /// \return The converted value, if available.
  template <typename T>
    requires detail::has_object_convert<T>
  [[nodiscard]] auto try_as() const -> std::optional<T> {
    if (!state_ || state_->size < 2 || state_->bid.pool == detail::dhandle_t{}) {
      return std::nullopt;
    }

    try {
      auto cs = get_cursor();
      return detail::object_value<T>::convert(cs);
    } catch (const dmn::error&) {
      return std::nullopt;
    }
  }

  /// Convert the object to a type.
  ///
  /// \return The converted value
  /// \throws dmn::conversion_error If the object can not be converted to `T`.
  template <typename T>
    requires detail::has_object_typecheck<T>
  [[nodiscard]] auto as() const -> T {
    auto result = try_as<T>();
    if (!result) {
      throw dmn::conversion_error("item_value cannot be converted to requested type");
    }

    return std::move(*result);
  }

  /// Convert the object to a string regardless of it's type
  ///
  /// \return The converted string, if available.
  /// \todo Add dmn::time_date conversion
  [[nodiscard]] auto as_string() const -> std::optional<std::string>;

  /// Get cursor pointing to object memory
  [[nodiscard]] auto get_cursor() const -> detail::locker;

 private:
  std::optional<detail::block_id> item_bid_ = std::nullopt;
  std::shared_ptr<void> owner_;
  std::shared_ptr<state> state_;

  /// Create object from raw domino memory whilst borrowing it.
  ///
  /// \param bid Instance of `detail::block_id` holding the raw memory.
  /// \param size Size of the raw memory.
  /// \param owner Instance that owns the raw memory.
  /// \note Text lists must be type-prefixed which means view entries aren't supported.
  template <class T>
  object(
    detail::block_id bid, size_t size, std::shared_ptr<T> owner,
    std::optional<detail::block_id> item_bid = std::nullopt
  )
      : item_bid_(item_bid),
        owner_(std::static_pointer_cast<void>(std::move(owner))),
        state_(std::make_shared<state>(state(bid, size))) {}

  [[nodiscard]] auto ensure_state() -> state&;
  [[nodiscard]] auto ensure_state() const -> const state&;

  [[nodiscard]] auto data_pair() const -> std::pair<dmn::type, detail::locker>;

  friend class dmn::note;
  friend class dmn::view;
};
}  // namespace dmn