#pragma once

#include <cstdint>

namespace dmn {
enum class item_flag : uint16_t {
  none = 0,
  sign = 1,
  seal = 2,
  summary = 4,
  readwriters = 32,
  names = 64,
  placeholder = 256,
  protect = 512,
  readers = 1024,
};
}  // namespace dmn