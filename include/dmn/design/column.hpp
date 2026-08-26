#pragma once

#include <string>

namespace dmn::design {
class view;

class column {
 public:
  auto set_title(std::string_view title) -> column&;
  auto set_formula(std::string_view formula) -> column&;

 private:
  std::string formula_;
  std::string item_name_;
  std::string title_;

  friend class view;
};
}  // namespace dmn::design
