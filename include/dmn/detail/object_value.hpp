#pragma once

#include <type_traits>

#include "dmn/formula.hpp"
#include "dmn/list.hpp"
#include "dmn/time_date.hpp"

namespace dmn::detail {
template <typename T>
struct object_value;

template <typename T>
concept has_object_convert = requires(detail::cursor& cs) {
  { object_value<T>::convert(cs) } -> std::same_as<std::optional<T>>;
};

template <typename T>
concept has_object_typecheck = requires(detail::cursor& cs) {
  { object_value<T>::is(cs) } -> std::same_as<bool>;
};

template <>
struct object_value<std::string> {
  static auto convert(detail::cursor& cs) -> std::optional<std::string>;
  static auto is(detail::cursor& cs) -> bool;
};

template <>
struct object_value<double> {
  static auto convert(detail::cursor& cs) -> std::optional<double>;
  static auto is(detail::cursor& cs) -> bool;
};

template <>
struct object_value<dmn::time_date> {
  static auto convert(detail::cursor& cs) -> std::optional<dmn::time_date>;
  static auto is(detail::cursor& cs) -> bool;
};

template <>
struct object_value<dmn::list> {
  static auto convert(detail::cursor& cs) -> std::optional<dmn::list>;
  static auto is(detail::cursor& cs) -> bool;
};

template <>
struct object_value<dmn::formula> {
  static auto convert(detail::cursor& cs) -> std::optional<dmn::formula>;
  static auto is(detail::cursor& cs) -> bool;
};

template <typename T>
  requires std::is_arithmetic_v<T> && (!std::is_same_v<T, double>)
struct object_value<T> {
  static auto convert(detail::cursor& cs) -> std::optional<T> {
    auto val = object_value<double>::convert(cs);
    if (val) {
      return static_cast<T>(*val);
    }
    return std::nullopt;
  }

  static auto is(detail::cursor& cs) -> bool { return object_value<double>::is(cs); }
};
}  // namespace dmn::detail
