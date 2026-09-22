#pragma once

#include <string_view>

#include "dmn/detail/locker.hpp"
#include "dmn/detail/runtime.hpp"
#include "dmn/detail/uhandle.hpp"

namespace dmn {
class lotusscript : protected detail::runtime {
 public:
  using handle_t = detail::dhandle_t;

  /// Create a formatted LotusScript object from code.
  ///
  /// \throws dmn::native_error If the creation failed.
  /// \throws dmn::formatting_error If the formatter encoutered a problem.
  explicit lotusscript(std::string_view code);

  /// Get LotusScript code size including null terminator.
  [[nodiscard]] auto size() const -> size_t { return size_; }

  [[nodiscard]] auto get_cursor() const -> detail::locker {
    return {get_handle(), size(), detail::ownership::borrow};
  }

  [[nodiscard]] auto get_handle() const -> handle_t { return hdl_.get(); }

 private:
  detail::uhandle<handle_t> hdl_;
  size_t size_;
};
}  // namespace dmn
