#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "dmn/detail/uhandle.hpp"

namespace dmn {
namespace design {
class view;
}

class note;
class object;

class formula {
 public:
  using handle_t = detail::dhandle_t;

  static auto compile(std::string_view command, std::string name = {}) -> formula;

  [[nodiscard]] auto decompile(bool is_selection_formula = false) const -> std::string;
  [[nodiscard]] auto size() const -> size_t;
  void merge(const formula& other) const;
  void add_summary(std::string_view item_name) const;

  [[nodiscard]] auto get_handle() const -> handle_t { return hdl_.get(); }

 private:
  detail::uhandle<handle_t> hdl_;

  formula(std::span<uint8_t> buffer);
  formula();

  friend class dmn::design::view;
  friend class dmn::note;
  friend class dmn::object;
};
}  // namespace dmn
