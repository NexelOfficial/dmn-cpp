#pragma once

#include <cstdint>
#include <cstring>
#include <span>
#include <type_traits>

#include "dmn/error.hpp"
#include "dmn/type.hpp"

template <typename T>
concept is_valid_string = std::is_convertible_v<T, std::basic_string<char>> ||
                          std::is_convertible_v<T, std::basic_string<unsigned char>>;

namespace dmn::detail {
class cursor {
 public:
  cursor(uint8_t* ptr, size_t size) noexcept : buffer_(ptr, size) {
    if (ptr == nullptr) {
      buffer_ = buffer_.subspan(0, 0);
    }
  };

  void reset() noexcept { offset_ = 0; }
  void reset(uint8_t* ptr, size_t size) noexcept {
    buffer_ = {ptr, size};
    offset_ = 0;
  }

  void write(const void* data, size_t size, dmn::type typ = dmn::type::unavailable);

  void read(void* out, size_t size, dmn::type typ = dmn::type::unavailable);

  template <typename T, dmn::type Type = dmn::type::unavailable>
    requires std::is_trivially_copyable_v<T>
  void write(const T& value) {
    write(&value, sizeof(T), Type);
  }

  template <typename T, dmn::type Type = dmn::type::unavailable>
    requires std::is_trivially_copyable_v<T>
  [[nodiscard]] auto read() -> T {
    T out{};
    read(&out, sizeof(T), Type);
    return out;
  }

  void advance_offset(size_t amount) noexcept { offset_ += amount; }

  void set_offset(size_t position) noexcept { offset_ = position; }

  [[nodiscard]] auto get_offset() const noexcept -> size_t { return offset_; }

  /// Get the pointer with a custom offset
  [[nodiscard]] auto get_pointer(size_t offset) const noexcept -> uint8_t* {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return buffer_.data() + offset;
  }

  /// Get the pointer with the current offset
  [[nodiscard]] auto get_pointer() const noexcept -> uint8_t* { return get_pointer(get_offset()); }

 private:
  std::span<uint8_t> buffer_;
  size_t offset_ = 0;

  void ensure_bounds(uint32_t size_to_read) const {
    const size_t new_offset = offset_ + size_to_read;
    if (new_offset > buffer_.size()) {
      throw dmn::out_of_range("Memory read out of range");
    }
  }

  template <typename>
  static constexpr bool always_false = false;
};
}  // namespace dmn::detail