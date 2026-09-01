#pragma once

#include <array>
#include <cstdint>

namespace dmn::design {
// NOLINTNEXTLINE(performance-enum-size) Colors are uint16_t
enum class color : uint16_t {
  black = 0,
  white = 1,
  red = 2,
  green = 3,
  blue = 4,
  magenta = 5,
  yellow = 6,
  cyan = 7,
  dark_red = 8,
  dark_green = 9,
  dark_blue = 10,
  dark_magenta = 11,
  dark_yellow = 12,
  dark_cyan = 13,
  gray = 14,
  light_gray = 15,
};

struct color_value {
  uint16_t flags = 0;
  std::array<uint8_t, 3> rgb{};
  uint8_t unused = 0;
};
}  // namespace dmn::design