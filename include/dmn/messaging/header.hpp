#include <string>
#include <string_view>

#include "dmn/error.hpp"

namespace dmn {
struct header {
  std::string key;
  std::string value;

  explicit constexpr header(std::pair<std::string_view, std::string_view> kv)
      : key(kv.first), value(kv.second) {
    constexpr std::string_view allowed =
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "0123456789!#$%&'*+-.^_`|~";

    constexpr uint8_t SPACE_CHAR = 0x20;
    constexpr uint8_t DELETE_CHAR = 0x7f;

    if (key.empty() || key.find_first_not_of(allowed) != std::string::npos) {
      throw dmn::invalid_argument("Invalid header key");
    }

    for (const uint8_t c : value) {
      if ((c < SPACE_CHAR && c != '\t') || c == DELETE_CHAR) {
        throw dmn::invalid_argument("Invalid header value");
      }
    }
  }
};
}  // namespace dmn