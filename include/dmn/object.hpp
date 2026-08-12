#pragma once

#include <cstddef>
#include <cstdint>

#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include "dmn/detail/lmbcs.hpp"
#include "dmn/detail/locker.hpp"
#include "dmn/error.hpp"
#include "dmn/time_date.hpp"
#include "dmn/list.hpp"
#include "dmn/type.hpp"

template <typename T>
concept is_item_value = std::is_same_v<T, std::string> || std::is_same_v<T, dmn::list> ||
                        std::is_same_v<T, dmn::time_date> || std::is_integral_v<T> ||
                        std::is_same_v<T, double> || std::is_same_v<T, bool>;

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
  /// \param typ Type of the data.
  /// \param data Data buffer to write.
  /// \throws dmn::runtime_error If the object is not an item value.
  /// \throws dmn::native_error If the reallocation failed.
  /// \note Writing data is not possible when the object does not belong to an item value.
  /// \note All copies of the object will point to the new memory.
  void write(dmn::type typ, std::span<const uint8_t> data);

  /// Check whether the object can be converted to a type.
  template <typename T>
    requires is_item_value<T>
  [[nodiscard]] auto is() const -> bool {
    if (!state_ || state_->size < 2 || state_->bid.pool == detail::dhandle_t{}) {
      return false;
    }

    detail::locker obj(state_->bid, state_->size, detail::ownership::borrow);
    auto typ = obj.read<dmn::type>();
    const size_t data_size = state_->size - sizeof(typ);

    if constexpr (std::is_same_v<T, std::string>) {
      return typ == dmn::type::text;
    } else if constexpr (std::is_same_v<T, bool>) {
      if (typ == dmn::type::number && data_size == sizeof(double)) {
        auto value = obj.read<double>();
        return value == 0 || value == 1;
      }
    } else if constexpr (std::is_convertible_v<T, double>) {
      return typ == dmn::type::number && data_size == sizeof(double);
    } else if constexpr (std::is_same_v<T, dmn::list>) {
      return typ == dmn::type::text_list && data_size >= sizeof(uint16_t);
    } else if constexpr (
      std::is_same_v<T, dmn::time_date> || std::is_same_v<T, std::chrono::system_clock::time_point>
    ) {
      return typ == dmn::type::time && data_size == sizeof(dmn::time_date);
    }

    return false;
  }

  /// Try to convert the object to a type.
  ///
  /// \return The converted value, if available.
  template <typename T>
    requires is_item_value<T>
  [[nodiscard]] auto try_as() const -> std::optional<T> {
    if (!state_ || state_->size < 2 || state_->bid.pool == detail::dhandle_t{}) {
      return std::nullopt;
    }

    try {
      detail::locker obj(state_->bid, state_->size, detail::ownership::borrow);
      const auto typ = obj.read<dmn::type>();
      const size_t data_size = state_->size - sizeof(typ);

      return try_convert<T>(obj, typ, data_size);
    } catch (dmn::error&) {
      return std::nullopt;
    }
  }

  /// Convert the object to a type.
  ///
  /// \return The converted value
  /// \throws dmn::conversion_error If the object can not be converted to `T`.
  template <typename T>
    requires is_item_value<T>
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

  template <typename T>
  [[nodiscard]] auto try_convert(detail::locker& obj, dmn::type typ, size_t data_size) const
    -> std::optional<T> {
    if constexpr (std::is_same_v<T, std::string>) {
      if (typ != dmn::type::text) {
        return std::nullopt;
      }

      // Use pointer with lmbcs::view instead of obj.read() to prevent double allocation
      auto* ptr = obj.get_pointer();
      const lmbcs::view value(ptr, data_size);
      return lmbcs::translate(value);
    } else if constexpr (std::is_same_v<T, bool>) {
      if (typ != dmn::type::number) {
        return std::nullopt;
      }

      auto value = obj.read<double>();
      if (value == 0 || value == 1) {
        return static_cast<bool>(value);
      }
    } else if constexpr (std::is_convertible_v<T, double>) {
      if (typ == dmn::type::number) {
        return static_cast<T>(obj.read<double>());
      }
    } else if constexpr (std::is_same_v<T, dmn::list>) {
      if (typ == dmn::type::text_list) {
        return dmn::list(obj.get_pointer(0));
      }
    } else if constexpr (std::is_same_v<T, dmn::time_date>) {
      if (typ == dmn::type::time) {
        return obj.read<dmn::time_date>();
      }
    }

    return std::nullopt;
  }

  friend class dmn::note;
  friend class dmn::view;
};
}  // namespace dmn