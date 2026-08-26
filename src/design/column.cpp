#include "dmn/design/column.hpp"

using dmn::design::column;

auto column::set_title(std::string_view title) -> column& {
  title_ = title;
  return *this;
}

auto column::set_formula(std::string_view formula) -> column& {
  formula_ = formula;
  return *this;
}