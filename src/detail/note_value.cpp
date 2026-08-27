#include "dmn/detail/note_value.hpp"

#include "dmn/detail/lmbcs.hpp"

using dmn::detail::note_value;

void note_value<std::string_view>::apply(std::string_view value, setter_func_t setter) {
  const auto converted = lmbcs::translate(value);
  const std::span span{converted.data(), converted.size()};
  std::invoke(setter, dmn::type::text, std::as_bytes(span));
}

void note_value<double>::apply(double value, setter_func_t setter) {
  const std::span span{&value, 1};
  std::invoke(setter, dmn::type::number, std::as_bytes(span));
}

void note_value<dmn::time_date>::apply(const dmn::time_date& value, setter_func_t setter) {
  const std::span span{&value, 1};
  std::invoke(setter, dmn::type::time, std::as_bytes(span));
}

void note_value<dmn::object>::apply(const dmn::object& value, setter_func_t setter) {
  auto cursor = value.get_cursor();
  const auto typ = cursor.read<dmn::type>();
  const std::span span{cursor.get_pointer(), cursor.size() - sizeof(uint16_t)};
  std::invoke(setter, typ, span);
}

void note_value<dmn::list>::apply(const dmn::list& value, setter_func_t setter) {
  const auto cursor = value.get_cursor();
  const std::span span{cursor.get_pointer(sizeof(uint16_t)), cursor.size() - sizeof(uint16_t)};
  std::invoke(setter, dmn::type::text_list, span);
}

void note_value<dmn::formula>::apply(const dmn::formula& value, setter_func_t setter) {
  const auto cursor = value.get_cursor();
  const std::span span{cursor.get_pointer(), cursor.size()};
  std::invoke(setter, dmn::type::formula, span);
}