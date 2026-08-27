#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace dmn {
class dql {
 public:
  struct date_time {
    std::string value;
  };

  using value_t =
    std::variant<std::string, const char*, int, long, long long, double, bool, date_time>;

  struct expression {
    enum class oper : uint8_t { comparison, contains, logical_and, logical_or, logical_not, raw };

    oper mode{oper::raw};

    std::string field;
    std::string op;
    value_t value{std::string{}};

    std::string raw;
    std::vector<expression> children;
  };

  /// Create a DQL datetime wrapper.
  ///
  /// \param value Raw DQL datetime string.
  /// \return Wrapped datetime value.
  /// \throws dmn::invalid_argument If the provided value is empty.
  static auto datetime(std::string value) -> date_time;

  /// Create an equality comparison expression.
  ///
  /// \param field Field identifier to compare.
  /// \param value Value to compare against.
  /// \return DQL equality expression.
  /// \throws dmn::invalid_argument If the field identifier is invalid.
  static auto eq(std::string field, value_t value) -> expression;

  /// Create a non-equality comparison expression.
  ///
  /// \param field Field identifier to compare.
  /// \param value Value to compare against.
  /// \return DQL non-equality expression.
  /// \throws dmn::invalid_argument If the field identifier is invalid.
  static auto ne(std::string field, value_t value) -> expression;

  /// Create a less-than comparison expression.
  ///
  /// \param field Field identifier to compare.
  /// \param value Value to compare against.
  /// \return DQL less-than expression.
  /// \throws dmn::invalid_argument If the field identifier is invalid.
  static auto lt(std::string field, value_t value) -> expression;

  /// Create a less-than-or-equal comparison expression.
  ///
  /// \param field Field identifier to compare.
  /// \param value Value to compare against.
  /// \return DQL less-than-or-equal expression.
  /// \throws dmn::invalid_argument If the field identifier is invalid.
  static auto lte(std::string field, value_t value) -> expression;

  /// Create a greater-than comparison expression.
  ///
  /// \param field Field identifier to compare.
  /// \param value Value to compare against.
  /// \return DQL greater-than expression.
  /// \throws dmn::invalid_argument If the field identifier is invalid.
  static auto gt(std::string field, value_t value) -> expression;

  /// Create a greater-than-or-equal comparison expression.
  ///
  /// \param field Field identifier to compare.
  /// \param value Value to compare against.
  /// \return DQL greater-than-or-equal expression.
  /// \throws dmn::invalid_argument If the field identifier is invalid.
  static auto gte(std::string field, value_t value) -> expression;

  /// Create a contains expression.
  ///
  /// \param field Field identifier to compare.
  /// \param text Text to search for.
  /// \return DQL contains expression.
  /// \throws dmn::invalid_argument If the field identifier is invalid or the text is empty.
  static auto contains(std::string field, std::string text) -> expression;

  /// Create a logical AND expression from multiple terms.
  ///
  /// \param terms Expressions to join.
  /// \return DQL logical AND expression.
  /// \throws dmn::invalid_argument If the provided list is empty.
  static auto all(std::vector<expression> terms) -> expression;

  /// Create a logical OR expression from multiple terms.
  ///
  /// \param terms Expressions to join.
  /// \return DQL logical OR expression.
  /// \throws dmn::invalid_argument If the provided list is empty.
  static auto any(std::vector<expression> terms) -> expression;

  /// Render an expression as a DQL query string.
  ///
  /// \param expr Expression to render.
  /// \return Rendered DQL string.
  /// \throws dmn::invalid_argument If the expression is malformed or the operator is unknown.
  static auto render(const expression& expr) -> std::string;

 private:
  static auto comparison(std::string field, std::string op, value_t value) -> expression;
  static void validate_field(std::string_view field);
  static auto is_valid_identifier(std::string_view value) -> bool;
  static auto render_identifier(std::string_view field) -> std::string;
  static auto render_value(const value_t& value) -> std::string;
  static auto quote(std::string_view value) -> std::string;
  static auto render_double(double value) -> std::string;
  static auto render_joined(const std::vector<expression>& children, std::string_view op)
    -> std::string;

  template <typename>
  static constexpr bool always_false = false;
};

}  // namespace dmn
