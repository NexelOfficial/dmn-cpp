#pragma once

#include <string>
#include <string_view>

#include "dmn/detail/locker.hpp"
#include "dmn/detail/uhandle.hpp"

namespace dmn {
namespace detail {
template <typename T>
struct object_value;
}

class formula {
  struct header {
    uint16_t length;
    uint16_t flags;
    uint16_t offset;
  };

 public:
  using handle_t = detail::dhandle_t;

  explicit formula(std::string_view command);

  [[nodiscard]] auto decompile(bool is_selection_formula = false) const -> std::string;
  [[nodiscard]] auto size() const -> size_t;
  
  void merge(const formula& other) const;
  void add_summary(std::string_view item_name) const;
  void add_summary(lmbcs::view item_name) const;
  void add_item_name(std::string_view item_name) const;
  void add_item_name(lmbcs::view item_name) const;

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
