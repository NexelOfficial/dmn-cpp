#pragma once

#include <string>
#include <string_view>

#include "dmn/detail/locker.hpp"
#include "dmn/detail/runtime.hpp"
#include "dmn/detail/uhandle.hpp"

namespace dmn {
namespace detail {
template <typename T>
struct object_value;
}

/// Compiled Domino formula object.
///
/// \throws dmn::invalid_handle If the underlying handle is empty.
/// \throws dmn::native_error In case of a lower level failure.
class formula : protected detail::runtime {
  struct header {
    uint16_t length;
    uint16_t flags;
    uint16_t offset;
  };

 public:
  using handle_t = detail::dhandle_t;

  /// Compile the provided code and create a Formula object.
  explicit formula(std::string_view code);

  /// Decompile the compiled code.
  ///
  /// \param is_selection_formula Whether the compiled formula is a selection formula.
  [[nodiscard]] auto decompile(bool is_selection_formula = false) const -> std::string;

  /// Get the size of the underlying memory.
  ///
  /// \param even Round up the size to an even number.
  [[nodiscard]] auto size(bool even = false) const -> size_t;

  /// Merge another Formula into this one.
  void merge(const formula& other) const;

  /// Add a summary item to the Formula.
  void add_summary(std::string_view item_name) const;
  
  /// Add a summary item to the Formula.
  void add_summary(dmn::lmbcs_view item_name) const;

  /// Add an item name to the formula.
  ///
  /// \throws dmn::runtime_error If the formula already has an item name.
  void add_item_name(std::string_view item_name) const;

  /// Add an item name to the formula.
  ///
  /// \throws dmn::runtime_error If the formula already has an item name.
  void add_item_name(dmn::lmbcs_view item_name) const;

  [[nodiscard]] auto get_cursor() const -> detail::locker {
    return {get_handle(), size(), detail::ownership::borrow};
  }

  [[nodiscard]] auto get_handle() const -> handle_t { return hdl_.get(); }

 private:
  detail::uhandle<handle_t> hdl_;

  formula(std::span<std::byte> buffer);
  formula(handle_t hdl);

  friend struct detail::object_value<dmn::formula>;
};
}  // namespace dmn
