#pragma once

#include <functional>
#include <span>
#include <string_view>
#include <type_traits>

#include "dmn/detail/locker.hpp"
#include "dmn/formula.hpp"
#include "dmn/list.hpp"
#include "dmn/time_date.hpp"
#include "dmn/type.hpp"
#include "dmn/value.hpp"

namespace dmn::detail {
using setter_func_t = std::function<void(dmn::type, std::span<const std::byte>)>;

template <typename T>
struct note_value;

template <typename T>
concept has_note_value_apply = requires(const T& value, setter_func_t setter) {
  { note_value<T>::apply(value, setter) } -> std::same_as<void>;
};

template <>
struct note_value<std::string_view> {
  static void apply(std::string_view value, setter_func_t setter);
};

template <>
struct note_value<double> {
  static void apply(double value, setter_func_t setter);
};

template <>
struct note_value<dmn::time_date> {
  static void apply(const dmn::time_date& value, setter_func_t setter);
};

template <>
struct note_value<dmn::list> {
  static void apply(const dmn::list& value, setter_func_t setter);
};

template <>
struct note_value<dmn::formula> {
  static void apply(const dmn::formula& value, setter_func_t setter);
};

template <typename T>
  requires std::derived_from<std::remove_cvref_t<T>, dmn::value_impl>
struct note_value<T> {
  static void apply(const T& value, setter_func_t setter) {
    detail::locker cursor = value.get_cursor();
    const auto typ = cursor.read<dmn::type>();
    const std::span span{cursor.get_pointer(), cursor.size() - sizeof(dmn::type)};
    std::invoke(setter, typ, span);
  }
};

template <typename T>
  requires std::convertible_to<T, std::string_view> && (!std::same_as<T, std::string_view>)
struct note_value<T> {
  static void apply(const T& value, setter_func_t setter) {
    if constexpr (std::is_array_v<T>) {
      note_value<std::string_view>::apply(
        std::string_view{std::data(value), std::size(value) - 1}, std::move(setter)
      );
    } else {
      note_value<std::string_view>::apply(std::string_view{value}, std::move(setter));
    }
  }
};

template <typename T>
  requires std::is_arithmetic_v<T> && (!std::same_as<T, double>)
struct note_value<T> {
  static void apply(const T& value, setter_func_t setter) {
    note_value<double>::apply(static_cast<double>(value), std::move(setter));
  }
};
}  // namespace dmn::detail