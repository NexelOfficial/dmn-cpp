#include "dmn/design/column.hpp"

#include <domino/global.h>
#include <domino/viewfmt.h>

using dmn::design::column;
using dmn::design::view_column_format;
using dmn::design::view_column_format2;

static_assert(sizeof(view_column_format) == sizeof(VIEW_COLUMN_FORMAT));
static_assert(alignof(view_column_format) == alignof(VIEW_COLUMN_FORMAT));

static_assert(sizeof(view_column_format2) == sizeof(VIEW_COLUMN_FORMAT2));
static_assert(alignof(view_column_format2) == alignof(VIEW_COLUMN_FORMAT2));

constexpr uint8_t DEFAULT_COLUMN_WIDTH = 80;

view_column_format::view_column_format()
    : signature_(VIEW_COLUMN_FORMAT_SIGNATURE),
      display_width_(DEFAULT_COLUMN_WIDTH),
      font_id_(design::font{font::style::bold}.get_font_id()),
      format_data_type_(VIEW_COL_TEXT) {};

view_column_format2::view_column_format2()
    : header_font_id_(design::font{}.get_font_id()), signature_(VIEW_COLUMN_FORMAT_SIGNATURE2) {};

auto column::set_item_name(std::string_view item_name) -> column& {
  item_name_ = dmn::lmbcs::from_string(item_name);
  return *this;
}

auto column::set_title(std::string_view title) -> column& {
  title_ = dmn::lmbcs::from_string(title);
  return *this;
}

auto column::set_formula(dmn::formula formula) -> column& {
  formula_ = std::move(formula);
  return *this;
}

auto column::set_header_font(design::font font) -> column& {
  format2_.header_font_id_ = font.get_font_id();
  return *this;
}

auto column::set_item_font(design::font font) -> column& {
  format_.font_id_ = font.get_font_id();
  return *this;
}

auto column::set_sorting(sorting sort, bool categorized) -> column& {
  constexpr WORD sort_mask = VCF1_M_Sort | VCF1_M_SortCategorize | VCF1_M_SortDescending;
  format_.flags1_ &= ~sort_mask;

  if (sort != sorting::none || categorized) {
    format_.flags1_ |= VCF1_M_Sort;
  }

  if (sort == sorting::descending) {
    format_.flags1_ |= VCF1_M_SortDescending;
  }

  if (categorized) {
    format_.flags1_ |= VCF1_M_SortCategorize;
  }

  return *this;
}

auto column::get_sorting() const -> std::pair<sorting, bool> {
  const auto flags = format_.flags1_;
  const auto categorized = (flags & VCF1_M_SortCategorize) != 0;

  if ((flags & VCF1_M_Sort) == 0) {
    return {sorting::none, categorized};
  }

  const auto descending = (flags & VCF1_M_SortDescending) != 0;
  return {descending ? sorting::descending : sorting::ascending, categorized};
}