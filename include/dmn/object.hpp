#pragma once

#include <cstddef>

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "dmn/detail/thread_bound.hpp"
#include "dmn/detail/object_value.hpp"
#include "dmn/detail/locker.hpp"
#include "dmn/type.hpp"

namespace dmn {
class note;

class object : protected detail::runtime {
 public:
  object() = default;

  /// Create object from `detail::locker`.
  ///
  /// \param locker Instance of `detail::locker` holding the raw memory.
  /// \throws dmn::invalid_argument If the locker does not own the memory.
  object(detail::locker locker);

  /// Check whether the object is empty.
  ///
  /// An object is considered empty when its size is two bytes or less. At that point it's
  /// either completely empty or only has a type but no data.
  [[nodiscard]] auto empty() const noexcept -> bool;

  /// Extract the type of the object.
  [[nodiscard]] auto get_type() const -> dmn::type;

  /// Check whether the object can be converted to a type.
  template <typename T>
    requires detail::has_object_typecheck<T>
  [[nodiscard]] auto is() const -> bool {
    if (size_ < sizeof(dmn::type) || *bid_ == detail::block_id{}) {
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
    if (size_ < 2 || *bid_ == detail::block_id{}) {
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
  std::shared_ptr<void> owner_;
  detail::thread_bound<detail::block_id> bid_;
  detail::thread_bound<detail::block_id> item_bid_;
  size_t size_ = 0;

  /// Create object from raw domino memory whilst borrowing it.
  ///
  /// \param bid Block ID pointing to the raw memory.
  /// \param size Size of the raw memory.
  /// \param owner Instance that owns the raw memory.
  /// \param item_bid Optional Block ID that belongs to the item.
  /// \note Text list values must be type-prefixed
  template <class T>
  object(
    detail::block_id bid, size_t size, std::shared_ptr<T> owner,
    std::optional<detail::block_id> item_bid = std::nullopt
  )
      : owner_(std::static_pointer_cast<void>(std::move(owner))),
        bid_(bid),
        size_(size),
        item_bid_(item_bid.value_or({})) {}

  [[nodiscard]] auto data_pair() const -> std::pair<dmn::type, detail::locker>;

  friend class dmn::note;
};
}  // namespace dmn