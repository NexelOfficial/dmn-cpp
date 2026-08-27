#pragma once

#include <cstdint>
#include <string_view>

#include "dmn/design/color.hpp"
#include "dmn/detail/lmbcs.hpp"
#include "dmn/formula.hpp"
#include "dmn/unid.hpp"

namespace dmn::design {
class column;
class view;

struct number_format {
  uint8_t digits;
  uint8_t format;
  uint8_t attributes;
  uint8_t unused;
};

struct time_format {
  uint8_t date;
  uint8_t time;
  uint8_t zone;
  uint8_t structure;
};

class view_column_format {
 public:
  view_column_format();

 private:
  uint16_t signature_;
  uint16_t flags1_ = 0;
  uint16_t item_name_size_ = 0;
  uint16_t title_size_ = 0;
  uint16_t formula_size_ = 0;
  uint16_t constant_value_size_ = 0;
  uint16_t display_width_;
  uint32_t font_id_;
  uint16_t flags2_ = 0;
  number_format number_format_{};
  time_format time_format_{};
  uint16_t format_data_type_;
  uint16_t list_separator_ = 0;

  friend class column;
  friend class view;
};

class view_column_format2 {
 public:
  view_column_format2();

 private:
  uint16_t signature_;
  uint32_t header_font_id_ = 0;
  dmn::unid resort_to_view_unid_{};
  uint16_t second_resort_column_index_ = 0;
  uint16_t flags3_ = 0;
  uint16_t hide_when_formula_size_ = 0;
  uint16_t twistie_resource_size_ = 0;
  uint16_t custom_order_ = 0;
  uint16_t custom_hidden_flags_ = 0;
  design::color_value column_color_{};
  design::color_value header_font_color_{};

  friend class column;
  friend class view;
};

class column {
 public:
  column() : formula_(dmn::formula{"@DocNumber"}) {};

  auto set_item_name(std::string_view item_name) -> column&;
  auto set_title(std::string_view title) -> column&;
  auto set_formula(dmn::formula formula) -> column&;

 private:
  view_column_format format_;
  view_column_format2 format2_;
  dmn::formula formula_;
  lmbcs::str item_name_;
  lmbcs::str title_;

  friend class view;
};
}  // namespace dmn::design
