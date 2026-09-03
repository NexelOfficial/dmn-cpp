#pragma once

#include <string>
#include <string_view>

namespace dmn {

class lmbcs : public std::basic_string<unsigned char> {
 public:
  using base_type = std::basic_string<unsigned char>;
  using char_t = unsigned char;

  /// Construct a LMBCS without conversion.
  lmbcs(std::string_view str)
      : base_type(reinterpret_cast<const char_t*>(str.data()), str.size()) {};

  lmbcs() = default;

  /// Translate the provided UTF-8 std::string to LMBCS.
  [[nodiscard]] static auto from_string(std::string_view str) -> lmbcs;

  /// Translate the current LMBCS to a UTF-8 std::string.
  [[nodiscard]] auto to_string() const -> std::string;

  /// Translate the current LMBCS to a UTF-8 std::string.
  [[nodiscard]] auto to_string() -> std::string;

  /// Accessor for c_str() but as a custom type.
  template <typename T = char>
  [[nodiscard]] auto c_str() const noexcept -> const T* {
    return reinterpret_cast<const T*>(base_type::c_str());
  }

  /// Accessor for data() but as a custom type.
  template <typename T = char>
  [[nodiscard]] auto data() noexcept -> T* {
    return reinterpret_cast<T*>(base_type::data());
  }
};

class lmbcs_view : public std::basic_string_view<unsigned char> {
 public:
  using base_type = std::basic_string_view<unsigned char>;
  using char_t = unsigned char;

  lmbcs_view(base_type str) : lmbcs_view(str.data(), str.size()) {};

  lmbcs_view(const char_t* ptr, size_t size) : base_type(ptr, size) {};

  lmbcs_view(const char* ptr, size_t size)
      : base_type(reinterpret_cast<const char_t*>(ptr), size) {};

  lmbcs_view(const lmbcs& str) : base_type(str) {};

  /// Translate the current LMBCS to a UTF-8 std::string.
  [[nodiscard]] auto to_string() const -> std::string;

  /// Translate the current LMBCS to a UTF-8 std::string.
  [[nodiscard]] auto to_string() -> std::string;

  /// Accessor for data() but as a custom type.
  template <typename T = char>
  auto data() noexcept -> const T* {
    return reinterpret_cast<const T*>(base_type::data());
  }
};
}  // namespace dmn