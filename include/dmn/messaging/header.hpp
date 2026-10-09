#include <limits>
#include <string>
#include <string_view>

#include "dmn/detail/random.hpp"
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

class multipart {
  constexpr static uint8_t BOUNDARY_SIZE = std::numeric_limits<uint8_t>::max();

 public:
  explicit multipart() : boundary_(detail::random_string(BOUNDARY_SIZE)) {};

  [[nodiscard]] auto header() const -> dmn::header {
    return dmn::header{{"Content-Type", "multipart/mixed; boundary=\"" + boundary_ + "\""}};
  };

  [[nodiscard]] auto next_boundary() const -> std::string { return "--" + boundary_ + "\r\n"; }

  [[nodiscard]] auto end_boundary() const -> std::string { return "--" + boundary_ + "--"; }

 private:
  std::string boundary_;
};
}  // namespace dmn