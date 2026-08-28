#pragma once

#include <cstdint>
#include <array>

#include "dmn/design/color.hpp"

namespace dmn::design {

class font {
  constexpr static uint8_t MAX_STYLES = 6;

 public:
  struct id {
    uint32_t value;
  };

  enum class style : uint8_t {
    none = 0,
    bold = 1,
    italic = 2,
    underline = 4,
    strikethrough = 8,
  };

  enum class size : uint8_t {
    xsmall = 8,
    small = 9,
    normal = 10,
    large = 11,
    xlarge = 12,
    xxlarge = 14
  };

  template <typename... Args>
    requires(
      (std::same_as<Args, style> || std::same_as<Args, color> || std::same_as<Args, size>) && ...
    )
  constexpr font(Args... args) {
    size_t i = 0;
    auto push = [&]<typename T>(T value) {
      if constexpr (std::same_as<T, style>) {
        styles_.at(i++) = value;
      } else if constexpr (std::same_as<T, size>) {
        size_ = value;
      } else {
        color_ = value;
      }
    };

    (push(args), ...);
  }

  /// Extract the font id for this font.
  [[nodiscard]] auto get_font_id() const -> id;

 private:
  std::array<style, MAX_STYLES> styles_{style::none};
  size size_ = size::normal;
  color color_ = color::black;
};
}  // namespace dmn::design