#pragma once

#include <cstdint>
#include <cstring>
#include <span>
#include <type_traits>

#include "dmn/detail/ods.hpp"
#include "dmn/error.hpp"

template <typename T>
concept is_container_like = requires(T t) {
  { std::size(t) } -> std::same_as<size_t>;
  { std::data(t) };
};

namespace dmn::detail {
class cursor {
 public:
  cursor(std::byte* ptr, size_t size) noexcept : buffer_(ptr, size) {
    if (ptr == nullptr) {
      buffer_ = buffer_.subspan(0, 0);
    }
  };

  void reset() noexcept { offset_ = 0; }
  void reset(std::byte* ptr, size_t size) noexcept {
    buffer_ = {ptr, size};
    offset_ = 0;
  }

  template <typename T>
    requires std::is_trivially_copyable_v<T>
  void write(std::span<T> buffer) {
    auto bytes = std::as_bytes(buffer);
    ensure_bounds(bytes.size());
    std::memcpy(get_pointer(), bytes.data(), bytes.size());
    offset_ += bytes.size();
  }

  template <typename T>
    requires std::is_trivially_copyable_v<T>
  void read(std::span<T> buffer) {
    auto bytes_size = std::as_bytes(buffer).size();
    ensure_bounds(bytes_size);
    std::memcpy(buffer.data(), get_pointer(), bytes_size);
    offset_ += bytes_size;
  }

  template <typename T>
    requires std::is_trivially_copyable_v<T> && (!is_container_like<T>)
  void write(const T& value, std::optional<detail::ods::type> typ = std::nullopt) {
    auto size = typ ? detail::ods::size(*typ) : sizeof(T);
    ensure_bounds(size);

    if (typ) {
      detail::ods::write(get_pointer(), &value, *typ);
    } else {
      std::memcpy(get_pointer(), &value, size);
    }
    offset_ += size;
  }

  template <typename T>
    requires std::is_trivially_copyable_v<T> && (!is_container_like<T>)
  [[nodiscard]] auto read(std::optional<detail::ods::type> typ = std::nullopt) -> T {
    auto size = typ ? detail::ods::size(*typ) : sizeof(T);
    ensure_bounds(size);

    T out{};
    if (typ) {
      detail::ods::read(&out, get_pointer(), *typ);
    } else {
      std::memcpy(&out, get_pointer(), size);
    }
    offset_ += size;
    return out;
  }

  void advance_offset(size_t amount) noexcept { offset_ += amount; }

  void set_offset(size_t position) noexcept { offset_ = position; }

  [[nodiscard]] auto get_offset() const noexcept -> size_t { return offset_; }

  /// Get the pointer with a custom offset
  template <typename T = std::byte>
  [[nodiscard]] auto get_pointer(size_t offset) const noexcept -> T* {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<T*>(buffer_.data() + offset);
  }

  /// Get the pointer with the current offset
  template <typename T = std::byte>
  [[nodiscard]] auto get_pointer() const noexcept -> T* {
    return get_pointer<T>(get_offset());
  }

  [[nodiscard]] auto size() const -> size_t { return buffer_.size(); }

 private:
  std::span<std::byte> buffer_;
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