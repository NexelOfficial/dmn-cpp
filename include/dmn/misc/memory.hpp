#pragma once

#include <cstdint>
#include <cstring>
#include <span>
#include <stdexcept>
#include <type_traits>

template <typename T>
concept is_valid_string = std::is_convertible_v<T, std::basic_string<char>> ||
                          std::is_convertible_v<T, std::basic_string<unsigned char>>;

namespace dmn::misc {
class memory {
 public:
  memory(uint8_t* ptr, size_t size) noexcept : buffer_(ptr, size) {
    if (ptr == nullptr) {
      buffer_ = buffer_.subspan(0, 0);
    }
  };

  void reset() noexcept { offset_ = 0; }
  void reset(uint8_t* ptr, size_t size) noexcept {
    buffer_ = {ptr, size};
    offset_ = 0;
  }

  void read(void* out, size_t size) {
    ensure_bounds(size);
    memcpy(out, buffer_.subspan(offset_, size).data(), size);
    offset_ += size;
  }

  template <typename T>
    requires requires(memory& obj) {
      { T::deserialize(obj) } -> std::same_as<T>;
    }
  [[nodiscard]] auto read() -> T {
    return T::deserialize(*this);
  }

  template <typename T>
  [[nodiscard]] auto read() -> T {
    if constexpr (std::is_trivially_copyable_v<T>) {
      T buff{};
      read(&buff, sizeof(T));
      return buff;
    } else if constexpr (is_valid_string<T>) {
      auto* ptr = buffer_.subspan(offset_).data();
      const size_t str_size = std::strlen(reinterpret_cast<char*>(ptr));

      T buff(str_size, '\0');
      read(buff.data(), str_size);
      increment_offset(1);
      return buff;
    } else {
      static_assert(std::false_type::value, "Object cannot be deserialized from memory object");
    }
  }

  void write(const void* data, size_t size) {
    ensure_bounds(size);
    memcpy(buffer_.subspan(offset_, size).data(), data, size);
    offset_ += size;
  }

  template <typename T>
    requires requires(const T t, memory& obj) {
      { t.serialize(obj) } -> std::same_as<void>;
    }
  void write(const T* data) {
    data->serialize(*this);
  }

  template <typename T>
  void write(const T* data) {
    if constexpr (std::is_trivially_copyable_v<T>) {
      write(data, sizeof(T));
    } else if constexpr (is_valid_string<T>) {
      write(data->data(), data->size());
      const char nul = '\0';
      write(&nul, 1);
    } else {
      static_assert(std::false_type::value, "Object cannot be serialized into memory object");
    }
  }

  [[nodiscard]] auto get_offset() const noexcept -> size_t { return offset_; }

  /// Get the pointer with a custom offset
  [[nodiscard]] auto get_pointer(size_t offset) const noexcept -> uint8_t* {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return buffer_.data() + offset;
  }

  /// Get the pointer with the current offset
  [[nodiscard]] auto get_pointer() const noexcept -> uint8_t* { return get_pointer(get_offset()); }

  void increment_offset(size_t by) noexcept { move_offset(offset_ + by); }

  void move_offset(size_t to) noexcept {
    if (to < buffer_.size()) {
      offset_ = to;
    }
  }

 private:
  std::span<uint8_t> buffer_;
  size_t offset_ = 0;

  void ensure_bounds(uint32_t size_to_read) {
    const size_t new_offset = offset_ + size_to_read;
    if (new_offset > buffer_.size()) {
      throw std::out_of_range("Memory read out of range");
    }
  }
};
}  // namespace dmn::misc