#pragma once

#include <cstddef>
#include <cstdint>

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include "dmn/nos/list.hpp"
#include "dmn/nos/type.hpp"
#include "dmn/os/lmbcs.hpp"
#include "dmn/os/locker.hpp"

template <typename T>
concept is_item_value =
  std::is_same_v<T, std::string> || std::is_same_v<T, dmn::list> || std::is_integral_v<T> ||
  std::is_same_v<T, double> || std::is_same_v<T, bool>;

namespace dmn {
class object {
 public:
  object() = default;

  /// Create object from raw domino memory whilst borrowing it.
  ///
  /// \param bid Instance of `dmn::os::block_id` holding the raw memory.
  /// \param size Size of the raw memory.
  /// \param owner Instance that owns the raw memory.
  /// \note Text lists must be type-prefixed which means view entries aren't supported.
  template <typename T>
  object(
    dmn::os::block_id bid, size_t size, std::shared_ptr<T> owner,
    std::optional<dmn::os::block_id> item_bid = std::nullopt
  )
      : bid_(bid),
        size_(size),
        owner_(std::static_pointer_cast<void>(std::move(owner))),
        item_bid_(item_bid) {}

  /// Check whether the object is empty.
  ///
  /// An object is considered empty when its size is two bytes or less. At that point it's
  /// either completely empty or only has a type but no data.
  [[nodiscard]] auto empty() const noexcept -> bool { return size_ <= 2; }

  /// Extract the type of the object.
  [[nodiscard]] auto get_type() const -> dmn::type {
    if (size_ < 2 || bid_.pool == dmn::dhandle_t{}) {
      return dmn::type::invalid_or_unknown;
    }

    dmn::os::locker obj(bid_, size_, dmn::os::ownership::borrow);
    return obj.read<dmn::type>();
  }

  /// Write data to the memory behind the object.
  ///
  /// \param typ Type of the data.
  /// \param data Data buffer to write.
  /// \throws std::runtime_error If the object is not an item value.
  /// \throws dmn::error If the reallocation failed.
  /// \note Writing data is not possible when the object does not belong to an item value.
  void write(dmn::type typ, std::span<uint8_t> data);

  /// Check whether the object can be converted to a type.
  template <typename T>
    requires is_item_value<T>
  [[nodiscard]] auto is() const -> bool {
    if (size_ < 2 || bid_.pool == dmn::dhandle_t{}) {
      return false;
    }

    dmn::os::locker obj(bid_, size_, dmn::os::ownership::borrow);
    auto typ = obj.read<dmn::type>();
    const size_t data_size = size_ - sizeof(typ);

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
    }

    return false;
  }

  /// Try to convert the object to a type.
  ///
  /// \return The converted value, if available.
  /// \throws std::out_of_range If the object doesn't hold enough data.
  /// \throws dmn::error If the conversion to `dmn::list` fails, only if `T` is `dmn::list`.
  template <typename T>
    requires is_item_value<T>
  [[nodiscard]] auto try_as() const -> std::optional<T> {
    if (size_ < 2 || bid_.pool == dmn::dhandle_t{}) {
      return std::nullopt;
    }

    dmn::os::locker obj(bid_, size_, dmn::os::ownership::borrow);
    auto typ = obj.read<dmn::type>();
    const size_t data_size = size_ - sizeof(typ);

    if constexpr (std::is_same_v<T, std::string>) {
      if (typ == dmn::type::text) {
        // Use pointer with lmbcs::view instead of obj.read() to prevent double allocation
        auto* ptr = obj.get_pointer();
        const lmbcs::view value(ptr, data_size);
        return lmbcs::translate(value);
      }
    } else if constexpr (std::is_same_v<T, bool>) {
      if (typ == dmn::type::number && data_size == sizeof(double)) {
        auto value = obj.read<double>();
        if (value == 0 || value == 1) {
          return static_cast<bool>(value);
        }
      }
    } else if constexpr (std::is_convertible_v<T, double>) {
      if (typ == dmn::type::number && data_size == sizeof(double)) {
        auto value = obj.read<double>();
        return static_cast<T>(value);
      }
    } else if constexpr (std::is_same_v<T, dmn::list>) {
      if (typ == dmn::type::text_list && data_size >= sizeof(uint16_t)) {
        return dmn::list(obj.get_pointer(0));
      }
    }

    return std::nullopt;
  }

  /// Convert the object to a type.
  ///
  /// \return The converted value
  /// \throws std::out_of_range If the object doesn't hold enough data.
  /// \throws std::runtime_error If the object can not be converted to `T`.
  /// \throws dmn::error If the conversion to `dmn::list` fails, only if `T` is `dmn::list`.
  template <typename T>
    requires is_item_value<T>
  [[nodiscard]] auto as() const -> T {
    auto result = try_as<T>();
    if (!result) {
      throw std::runtime_error{"item_value cannot be converted to requested type"};
    }

    return std::move(*result);
  }

  /// Convert the object to a string regardless of it's type
  ///
  /// \return The converted string, if available.
  /// \throws std::out_of_range If the object doesn't hold enough data.
  /// \throws dmn::error If the conversion to `dmn::list` fails, only if `T` is `dmn::list`.
  [[nodiscard]] auto as_string() const -> std::optional<std::string>;

 private:
  std::optional<dmn::os::block_id> item_bid_ = std::nullopt;
  std::shared_ptr<void> owner_;
  dmn::os::block_id bid_{};
  size_t size_{};
};
}  // namespace dmn