#pragma once

#include <string_view>

#include "dmn/design/column.hpp"
#include "dmn/formula.hpp"
#include "dmn/note.hpp"

namespace dmn {
class database;
}

namespace dmn::design {
class view {
 public:
  static auto open(const dmn::database& db, std::string_view title) -> std::optional<view>;
  static auto create(const dmn::database& db, std::string_view title) -> view;

  auto column(std::string_view title) -> design::column&;
  auto set_selection_formula(dmn::formula formula) -> view&;
  void save();

 private:
  dmn::note note_;
  dmn::formula selection_;
  std::vector<design::column> columns_;
  uint16_t next_sequence_ = 1;

  view(dmn::note note);

  static auto open_impl(dmn::note note) -> view;
  [[nodiscard]] auto build_view_format() -> std::vector<uint8_t>;

  friend class column;
};
}  // namespace dmn::design
