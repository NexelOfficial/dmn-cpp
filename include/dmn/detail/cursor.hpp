#pragma once

#include <cstdint>
#include <cstring>
#include <span>
#include <type_traits>

#include "dmn/detail/runtime.hpp"
#include "dmn/detail/ods.hpp"
#include "dmn/error.hpp"

template <typename T>
concept is_container_like = requires(T t) {
  { std::size(t) } -> std::same_as<size_t>;
  { std::data(t) };
};

namespace dmn::detail {
/// Cursor that can walk through a chunk of memory
///
/// \throws dmn::out_or_range If memory is accessed outside the cursor.
class cursor : protected detail::runtime {
 public:
  /// Create a cursor object using a pointer and size.
  ///
  /// \param ptr The pointer to view.
  /// \param size Size of the memory chunk.
  cursor(std::byte* ptr, size_t size) noexcept : buffer_(ptr, size) {
    if (ptr == nullptr) {
      buffer_ = buffer_.subspan(0, 0);
    }
  };

  /// Reset the offset back to zero.
  void reset() noexcept { offset_ = 0; }

  /// Start viewing another pointer and size and set offset to zero.
  void reset(std::byte* ptr, size_t size) noexcept {
    buffer_ = {ptr, size};
    offset_ = 0;
  }

  /// Write a chunk of data and advance the cursor forward.
  template <typename T>
    requires std::is_trivially_copyable_v<T>
  void write(std::span<T> buffer) {
    auto bytes = std::as_bytes(buffer);
    ensure_bounds(bytes.size());
    std::memcpy(get_pointer(), bytes.data(), bytes.size());
    offset_ += bytes.size();
  }

  /// Read a chunk of data and advance the cursor forward.
  template <typename T>
    requires std::is_trivially_copyable_v<T>
  void read(std::span<T> buffer) {
    auto bytes_size = std::as_bytes(buffer).size();
    ensure_bounds(bytes_size);
    std::memcpy(buffer.data(), get_pointer(), bytes_size);
    offset_ += bytes_size;
  }

  /// Write a trivially copyable object and advance the cursor forward.
  ///
  /// \param typ An optional ODS type to use ods::write instead of memcpy.
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

  /// Read a trivially copyable type and advance the cursor forward.
  ///
  /// \param typ An optional ODS type to use ods::read instead of memcpy.
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

  /// Advance the offset foward by an amount.
  void advance_offset(size_t amount) noexcept { offset_ += amount; }

  /// Manually set the offset to another position.
  void set_offset(size_t position) noexcept { offset_ = position; }

  /// Get the current offset.
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

  /// Get the size of the cursor memory.
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