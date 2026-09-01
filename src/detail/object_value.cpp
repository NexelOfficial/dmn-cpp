#include "dmn/detail/object_value.hpp"

#include "dmn/lmbcs.hpp"
#include "dmn/type.hpp"

using dmn::detail::object_value;

auto object_value<std::string>::convert(detail::cursor& cs) -> std::optional<std::string> {
  if (!is(cs)) {
    return std::nullopt;
  }

  // Use pointer with dmn::lmbcs_view instead of obj.read() to prevent double allocation
  auto* ptr = cs.get_pointer<dmn::lmbcs::char_t>();
  const dmn::lmbcs_view value(ptr, cs.size() - sizeof(dmn::type));
  return value.to_string();
}

auto object_value<std::string>::is(detail::cursor& cs) -> bool {
  return cs.read<dmn::type>() == dmn::type::text;
}

auto object_value<double>::convert(detail::cursor& cs) -> std::optional<double> {
  if (is(cs)) {
    return cs.read<double>();
  }
  return std::nullopt;
}

auto object_value<double>::is(detail::cursor& cs) -> bool {
  return cs.size() == sizeof(double) + sizeof(dmn::type) &&
         cs.read<dmn::type>() == dmn::type::number;
}

auto object_value<dmn::time_date>::convert(detail::cursor& cs) -> std::optional<dmn::time_date> {
  if (is(cs)) {
    return cs.read<dmn::time_date>();
  }
  return std::nullopt;
}

auto object_value<dmn::time_date>::is(detail::cursor& cs) -> bool {
  return cs.size() == sizeof(dmn::time_date) + sizeof(dmn::type) &&
         cs.read<dmn::type>() == dmn::type::time;
}

auto object_value<dmn::list>::convert(detail::cursor& cs) -> std::optional<dmn::list> {
  if (is(cs)) {
    return dmn::list({cs.get_pointer(0), cs.size()});
  }
  return std::nullopt;
}

auto object_value<dmn::list>::is(detail::cursor& cs) -> bool {
  return cs.read<dmn::type>() == dmn::type::text_list;
}

auto object_value<dmn::formula>::convert(detail::cursor& cs) -> std::optional<dmn::formula> {
  if (is(cs)) {
    return dmn::formula({cs.get_pointer(), cs.size() - sizeof(dmn::type)});
  }
  return std::nullopt;
}

auto object_value<dmn::formula>::is(detail::cursor& cs) -> bool {
  return cs.read<dmn::type>() == dmn::type::formula;
}
