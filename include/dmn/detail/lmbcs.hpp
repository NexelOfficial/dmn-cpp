#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>

namespace dmn {
class lmbcs {
 public:
  using str = std::basic_string<uint8_t>;
  using view = std::basic_string_view<uint8_t>;

  template <typename T>
  using return_t = std::conditional_t<std::convertible_to<T, lmbcs::view>, std::string, lmbcs::str>;

  template <typename T>
  using view_t =
    std::conditional_t<std::convertible_to<T, lmbcs::view>, lmbcs::view, std::string_view>;

  /// Translate a string-like buffer between encodings.
  ///
  /// Converts `lmbcs::view` to `std::string` (UTF-8) and `std::string_view` to `lmbcs::str`
  /// (LMBCS).
  ///
  /// \param text Buffer to translate.
  /// \return Output string holding the translated data. The size may be up to twice the size of the
  /// input buffer.
  template <typename T>
    requires std::convertible_to<T, lmbcs::view> || std::convertible_to<T, std::string_view>
  static auto translate(const T& text) -> return_t<T> {
    if constexpr (std::is_pointer_v<T>) {
      if (text == nullptr) {
        return return_t<T>{};
      }
    }

    const view_t<T> input{text};
    if (input.empty()) {
      return return_t<T>{};
    }

    return_t<T> out(input.size() * 2, '\0');
    const auto* inp_ptr = reinterpret_cast<const char*>(input.data());
    auto* out_ptr = reinterpret_cast<char*>(out.data());

    const auto out_len =
      translate_impl({inp_ptr, input.size()}, {out_ptr, out.size()}, std::same_as<T, lmbcs::view>);
    out.resize(out_len);
    return out;
  }

  /// Cast an `lmbcs::str` to a `const char*`.
  ///
  /// Uses a reinterpret cast to convert the underlying LMBCS pointer returned by
  /// `lmbcs::str::c_str()` to a `const char*`. Most Domino functions accept `char*` parameters even
  /// though they expect the string data to be encoded as LMBCS.
  static auto cast(const lmbcs::str& in) noexcept -> const char*;

  /// Cast an `lmbcs::str` to a `char*`.
  ///
  /// Uses a reinterpret cast to convert the underlying LMBCS pointer returned by
  /// `lmbcs::str::data()` to a `char*`. Most Domino functions accept `char*` parameters even though
  /// they expect the string data to be encoded as LMBCS.
  static auto cast(lmbcs::str& in) noexcept -> char*;

 private:
  /// Internal implementation used by `dmn::lmbcs::translate`.
  static auto translate_impl(std::span<const char> inp, std::span<char> out, bool to_utf8)
    -> size_t;
};
}  // namespace dmn