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
      font_id_(DEFAULT_BOLD_FONT_ID),
      format_data_type_(VIEW_COL_TEXT) {};

view_column_format2::view_column_format2() : signature_(VIEW_COLUMN_FORMAT_SIGNATURE2) {};

auto column::set_item_name(std::string_view item_name) -> column& {
  item_name_ = lmbcs::translate(item_name);
  return *this;
}

auto column::set_title(std::string_view title) -> column& {
  title_ = lmbcs::translate(title);
  return *this;
}

auto column::set_formula(dmn::formula formula) -> column& {
  formula_ = std::move(formula);
  return *this;
}