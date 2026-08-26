#pragma once

#include <functional>
#include <span>
#include <string_view>
#include <type_traits>

#include "dmn/formula.hpp"
#include "dmn/list.hpp"
#include "dmn/time_date.hpp"
#include "dmn/object.hpp"
#include "dmn/type.hpp"

namespace dmn::detail {
using setter_func_t = std::function<void(dmn::type, std::span<const std::byte>)>;

template <typename T>
struct note_value;

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
struct note_value<dmn::object> {
  static void apply(const dmn::object& value, setter_func_t setter);
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
  requires std::is_convertible_v<T, std::string_view> && (!std::is_same_v<T, std::string_view>)
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
  requires std::is_arithmetic_v<T> && (!std::is_same_v<T, double>)
struct note_value<T> {
  static void apply(const T& value, setter_func_t setter) {
    note_value<double>::apply(static_cast<double>(value), std::move(setter));
  }
};
}  // namespace dmn::detail