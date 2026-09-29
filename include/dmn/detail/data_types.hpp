#pragma once

#include <algorithm>
#include <limits>

#include "dmn/error.hpp"

namespace dmn::detail {
template <typename To, typename From>
/// Cast the provided value to a (smaller) type.
///
/// \throws std::overflow_error If the value does not fit the type.
constexpr auto checked_cast(From value) -> To {
  if (!std::in_range<To>(value)) {
    throw dmn::overflow_error("Value is too large to fit inside the provided type");
  }
  return static_cast<To>(value);
}

/// Cast the provided value to a (smaller) type.
///
/// Clamps the value to the types maximum limit if it's bigger.
template <typename To, typename From>
constexpr auto saturating_cast(From value) noexcept -> To {
  return std::clamp(value, 0, std::numeric_limits<To>::max());
}
}  // namespace dmn::detail