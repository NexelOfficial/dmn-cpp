#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <string_view>

#include "dmn/detail/runtime.hpp"
#include "dmn/design/column.hpp"
#include "dmn/design/color.hpp"
#include "dmn/formula.hpp"
#include "dmn/note.hpp"
#include "dmn/value.hpp"

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

/// Domino view design element.
///
/// \throws dmn::invalid_handle If an underlying handle is empty.
/// \throws dmn::native_error In case of a lower level failure.
class view : private detail::runtime {
 public:
  /// Create a view design element.
  ///
  /// \throws dmn::runtime_error If a view with the title already exists.
  static auto create(const dmn::database& db, std::string_view title) -> view;

  /// Access or create a column.
  auto column(std::string_view title) -> design::column&;

  /// Set the selection formula.
  auto set_selection_formula(dmn::formula formula) -> view&;

  /// Set the background color.
  auto set_background_color(design::color color) -> view&;

  /// Save the view design.
  ///
  /// \throws dmn::runtime_error If a column formula is misconfigured.
  void save();

 private:
  dmn::note note_;
  dmn::formula selection_;
  view_table_format table_format_{};
  view_table_format2 table_format2_{};
  std::deque<design::column> columns_;

  view(dmn::note note);

  static auto open_impl(dmn::note note) -> view;
  [[nodiscard]] auto build_view_format() -> dmn::value;
  [[nodiscard]] auto build_collation() const -> dmn::value;
};
}  // namespace dmn::design