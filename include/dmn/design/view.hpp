#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "dmn/design/column.hpp"
#include "dmn/design/color.hpp"
#include "dmn/formula.hpp"
#include "dmn/note.hpp"
#include "dmn/object.hpp"

namespace dmn {
class database;
}

namespace dmn::design {
struct view_table_format {
  std::array<uint8_t, 2> header;
  uint16_t columns;
  uint16_t sequence_number;
  uint16_t flags;
  uint16_t flags2;
};

struct view_table_format2 {
  uint16_t length;
  design::color background_color;
  design::color v2_border_color;
  font::id title_font;
  font::id unread_font;
  font::id totals_font;
  uint16_t auto_update_seconds;
  design::color alternate_background_color;
  uint16_t signature;
  uint8_t line_count;
  uint8_t spacing;
  design::color background_color_ext;
  uint8_t header_line_count;
  uint8_t flags1;
  std::array<uint16_t, 4> spare;
};

class view {
 public:
  static auto open(const dmn::database& db, std::string_view title) -> std::optional<view>;
  static auto create(const dmn::database& db, std::string_view title) -> view;

  auto column(std::string_view title) -> design::column&;
  auto set_selection_formula(dmn::formula formula) -> view&;
  auto set_background_color(design::color color) -> view&;
  void save();

 private:
  dmn::note note_;
  dmn::formula selection_;
  view_table_format table_format_{};
  view_table_format2 table_format2_{};
  std::vector<design::column> columns_;

  view(dmn::note note);

  static auto open_impl(dmn::note note) -> view;
  [[nodiscard]] auto build_view_format() -> dmn::object;
  [[nodiscard]] auto build_collation() const -> dmn::object;
};
}  // namespace dmn::design