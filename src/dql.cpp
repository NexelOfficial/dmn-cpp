#include "dmn/dql.hpp"

#include <algorithm>
#include <ranges>
#include <cctype>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace dmn {
auto dql::datetime(std::string value) -> date_time {
  if (value.empty()) {
    throw std::invalid_argument("DQL datetime value cannot be empty");
  }

  return {std::move(value)};
}

auto dql::eq(std::string field, value_t value) -> expression {
  return comparison(std::move(field), "=", std::move(value));
}

auto dql::ne(std::string field, value_t value) -> expression {
  return {
    .mode = expression::oper::logical_not,
    .children = {eq(std::move(field), std::move(value))},
  };
}

auto dql::lt(std::string field, value_t value) -> expression {
  return comparison(std::move(field), "<", std::move(value));
}

auto dql::lte(std::string field, value_t value) -> expression {
  return comparison(std::move(field), "<=", std::move(value));
}

auto dql::gt(std::string field, value_t value) -> expression {
  return comparison(std::move(field), ">", std::move(value));
}

auto dql::gte(std::string field, value_t value) -> expression {
  return comparison(std::move(field), ">=", std::move(value));
}

auto dql::contains(std::string field, std::string text) -> expression {
  validate_field(field);

  if (text.empty()) {
    throw std::invalid_argument("DQL contains text cannot be empty");
  }

  return {
    .mode = expression::oper::contains,
    .field = std::move(field),
    .value = std::move(text),
  };
}

auto dql::all(std::vector<expression> terms) -> expression {
  if (terms.empty()) {
    throw std::invalid_argument("DQL all() requires at least one expression");
  }

  return {
    .mode = expression::oper::logical_and,
    .children = std::move(terms),
  };
}

auto dql::any(std::vector<expression> terms) -> expression {
  if (terms.empty()) {
    throw std::invalid_argument("DQL any() requires at least one expression");
  }

  return {
    .mode = expression::oper::logical_or,
    .children = std::move(terms),
  };
}

auto dql::render(const expression& expr) -> std::string {
  switch (expr.mode) {
    case expression::oper::comparison:
      return render_identifier(expr.field) + " " + expr.op + " " + render_value(expr.value);

    case expression::oper::contains:
      return render_identifier(expr.field) + " contains (" + render_value(expr.value) + ")";

    case expression::oper::logical_and:
      return render_joined(expr.children, " and ");

    case expression::oper::logical_or:
      return render_joined(expr.children, " or ");

    case expression::oper::logical_not:
      if (expr.children.size() != 1) {
        throw std::invalid_argument("DQL not expression requires exactly one child");
      }

      return "not " + render(expr.children.front());

    case expression::oper::raw:
      if (expr.raw.empty()) {
        throw std::invalid_argument("DQL raw expression cannot be empty");
      }

      return expr.raw;
  }

  throw std::logic_error("Unknown DQL expression kind");
}

auto dql::comparison(std::string field, std::string op, value_t value) -> expression {
  validate_field(field);

  return {
    .mode = expression::oper::comparison,
    .field = std::move(field),
    .op = std::move(op),
    .value = std::move(value),
  };
}

void dql::validate_field(std::string_view field) {
  if (!is_valid_identifier(field)) {
    throw std::invalid_argument("Invalid DQL field identifier: " + std::string(field));
  }
}

auto dql::is_valid_identifier(std::string_view value) -> bool {
  if (value.empty()) {
    return false;
  }

  auto is_alpha = [](uint8_t c) { return std::isalpha(c) || c == '_'; };
  auto is_alpha_num = [](uint8_t c) { return std::isalnum(c) || c == '_'; };

  if (!is_alpha(static_cast<uint8_t>(value.front()))) {
    return false;
  }

  return std::all_of(value.begin() + 1, value.end(), [&](char c) {
    return is_alpha_num(static_cast<uint8_t>(c));
  });
}

auto dql::render_identifier(std::string_view field) -> std::string {
  validate_field(field);
  return std::string(field);
}

auto dql::render_value(const value_t& value) -> std::string {
  return std::visit(
    [](const auto& item) -> std::string {
      using T = std::decay_t<decltype(item)>;

      if constexpr (std::is_same_v<T, std::string>) {
        return dql::quote(item);
      } else if constexpr (std::is_same_v<T, const char*>) {
        if (item == nullptr) {
          throw std::invalid_argument("DQL string value cannot be null");
        }

        return dql::quote(std::string_view(item));
      } else if constexpr (std::is_same_v<T, bool>) {
        return item ? "true" : "false";
      } else if constexpr (
        std::is_same_v<T, int> || std::is_same_v<T, long> || std::is_same_v<T, long long>
      ) {
        return std::to_string(item);
      } else if constexpr (std::is_same_v<T, double>) {
        return dql::render_double(item);
      } else if constexpr (std::is_same_v<T, date_time>) {
        return "@dt(" + dql::quote(item.value) + ")";
      } else {
        static_assert(dql::always_false<T>, "Unsupported DQL value type");
      }
    },
    value
  );
}

auto dql::quote(std::string_view value) -> std::string {
  std::string out;
  out.reserve(value.size() + 2);
  out.push_back('\'');

  for (const char c : value) {
    switch (c) {
      case '\'':
        out += "''";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        out.push_back(c);
        break;
    }
  }

  out.push_back('\'');
  return out;
}

auto dql::render_double(double value) -> std::string {
  std::string out = std::to_string(value);

  while (!out.empty() && out.back() == '0') {
    out.pop_back();
  }

  if (!out.empty() && out.back() == '.') {
    out.pop_back();
  }

  return out;
}

auto dql::render_joined(const std::vector<expression>& children, std::string_view op)
  -> std::string {
  if (children.empty()) {
    throw std::invalid_argument("DQL group expression cannot be empty");
  }

  std::string out;
  for (const auto& [i, child] : children | std::views::enumerate) {
    if (i > 0) {
      out += op;
    }

    const bool needs_parens =
      child.mode == expression::oper::logical_and || child.mode == expression::oper::logical_or;

    if (needs_parens) {
      out += "(";
    }

    out += render(child);

    if (needs_parens) {
      out += ")";
    }
  }

  return out;
}
}  // namespace dmn
