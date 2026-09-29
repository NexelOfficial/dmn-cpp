#pragma once

#include <cstdint>
#include <array>
#include <chrono>
#include <optional>

#include "dmn/detail/runtime.hpp"

struct tagTIMEDATE;

namespace dmn {
struct time_date : protected detail::runtime {
  std::array<uint32_t, 2> innards{};

  /// Convert a system clock time point to time-date.
  ///
  /// \param tp Time point to convert
  /// \return Instance of `dmn::time_date`, if available.
  /// \throws dmn::conversion_error If the time point cannot be converted to a Domino time-date.
  /// \throws dmn::out_of_range If the time point is outside the calendar range.
  [[nodiscard]] static auto from_time_point(std::chrono::system_clock::time_point tp) -> time_date;

  /// Convert this time-date to a system clock time point.
  ///
  /// \return Time point, if available.
  [[nodiscard]] auto to_time_point() const -> std::optional<std::chrono::system_clock::time_point>;

  /// Cast to raw TIMEDATE structure.
  [[nodiscard]] auto as_raw_time_date() noexcept -> tagTIMEDATE*;

  auto operator==(time_date other) const noexcept -> bool { return innards == other.innards; }
};
}  // namespace dmn
