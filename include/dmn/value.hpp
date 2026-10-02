#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <utility>

#include "dmn/detail/block.hpp"
#include "dmn/detail/scoped_handle.hpp"
#include "dmn/detail/object_value.hpp"
#include "dmn/detail/locker.hpp"
#include "dmn/error.hpp"
#include "dmn/type.hpp"

namespace dmn {
class note;

class value_impl : protected detail::runtime {
 protected:
  value_impl() = default;
  ~value_impl() = default;

  value_impl(const value_impl&) = default;
  auto operator=(const value_impl&) -> value_impl& = default;
  value_impl(value_impl&&) noexcept = default;
  auto operator=(value_impl&&) -> value_impl& = default;

 public:
  /// Check whether the value is empty.
  ///
  /// A value is considered empty when its size is two bytes or less. At that point it's
  /// either completely empty or only has a type but no data.
  [[nodiscard]] auto empty() const -> bool;

  /// Extract the type of the value.
  [[nodiscard]] auto get_type() const -> dmn::type;

  /// Check whether the value can be converted to a type.
  template <typename T>
    requires detail::has_object_typecheck<T>
  [[nodiscard]] auto is() const -> bool {
    if (size() < sizeof(dmn::type)) {
      return false;
    }

    auto cs = get_cursor();
    return detail::object_value<T>::is(cs);
  }

  /// Try to convert the value to a type.
  ///
  /// \return The converted value, if available.
  template <typename T>
    requires detail::has_object_typecheck<T>
  [[nodiscard]] auto try_as() const -> std::optional<T> {
    if (size() < 2) {
      return std::nullopt;
    }

    try {
      auto cs = get_cursor();
      return detail::object_value<T>::convert(cs);
    } catch (const dmn::error&) {
      return std::nullopt;
    }
  }

  /// Convert the value to a type.
  ///
  /// \return The converted value
  /// \throws dmn::conversion_error If the value can not be converted to `T`.
  template <typename T>
    requires detail::has_object_typecheck<T>
  [[nodiscard]] auto as() const -> T {
    auto result = try_as<T>();
    if (!result) {
      throw dmn::conversion_error("value cannot be converted to requested type");
    }

    return std::move(*result);
  }

  /// Convert the value to a string regardless of it's type
  ///
  /// \return The converted string, if available.
  /// \todo Add dmn::time_date conversion
  [[nodiscard]] auto as_string() const -> std::optional<std::string>;

  /// Get cursor pointing to the underlying memory.
  [[nodiscard]] virtual auto get_cursor() const -> detail::locker = 0;

  /// Get the size of the underlying memroy.
  [[nodiscard]] virtual auto size() const -> size_t = 0;

 private:
  [[nodiscard]] auto data_pair() const -> std::pair<dmn::type, detail::locker>;

  friend class dmn::note;
};

/// Value viewing Domino memory.
///
/// \throws dmn::invalid_handle If an underlying handle is empty.
/// \throws dmn::native_error In case of a lower level failure.
class value final : public value_impl {
 public:
  using handle_t = detail::dhandle_t;

  /// Create value from `detail::locker`.
  ///
  /// \param locker Instance of `detail::locker` holding the raw memory.
  /// \throws dmn::invalid_argument If the locker does not own the memory.
  explicit value(detail::locker locker);

  [[nodiscard]] auto get_cursor() const -> detail::locker override {
    return {hdl_.get(), size_, detail::ownership::borrow};
  }

  [[nodiscard]] auto size() const -> size_t override { return size_; }

 private:
  detail::scoped_handle<handle_t> hdl_;
  size_t size_;
};

/// Value viewing a item inside a note
///
/// \throws dmn::invalid_handle If an underlying handle is empty.
/// \throws dmn::native_error In case of a lower level failure.
class item_value final : public value_impl {
 public:
  using handle_t = detail::dhandle_t;

  /// Create item value from note and item.
  ///
  /// \param note Instance of `dmn::note` which has the item set.
  /// \param item Item name to use in the item value.
  item_value(dmn::note note, dmn::lmbcs item);

  /// Go to the next item with the same name.
  ///
  /// \return True if a new item sits inside the item value; false otherwise.
  [[nodiscard]] auto next() -> bool;

  /// Go to the previous item with the same name.
  ///
  /// \return True if a new item sits inside the item value; false otherwise.
  [[nodiscard]] auto previous() -> bool;

  [[nodiscard]] auto get_cursor() const -> detail::locker override {
    return {value_bid_, size_, detail::ownership::borrow};
  }

  [[nodiscard]] auto size() const -> size_t override { return size_; }

 private:
  std::shared_ptr<dmn::note> note_;
  dmn::lmbcs item_;

  detail::block_id item_bid_;
  detail::block_id value_bid_;
  uint32_t size_ = 0;

  struct handles;
  void apply_handles(handles hdls);

  [[nodiscard]] auto move_impl(bool forward) -> bool;

  friend class dmn::note;
};
}  // namespace dmn