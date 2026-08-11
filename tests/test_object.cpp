#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#include "dmn/object.hpp"
#include "dmn/list.hpp"
#include "dmn/type.hpp"
#include "dmn/note.hpp"
#include "utils.hpp"

TEST_CASE("object conversion coverage", "[nos][object]") {
  auto [db, _] = utils::random_database();
  utils::note_guard note{db->create_note()};

  dmn::list tags{};
  tags.push_back("alpha");
  tags.push_back("beta");
  tags.push_back("gamma");

  const auto system_tp = std::chrono::system_clock::time_point{std::chrono::seconds{1769945696}};
  const auto now_td = dmn::time_date::from_time_point(system_tp);

  note->set("TextValue", "🐶🐶🐶");
  note->append("NumericValue", 42.5);
  note->append("FalseValue", false);
  note->append("TrueValue", true);
  note->append("DateValue", now_td.value());
  note->append("ListValue", tags);

  SECTION("default values are empty") {
    const dmn::object value{};

    REQUIRE(value.empty());
    REQUIRE_FALSE(value.is<std::string>());
    REQUIRE_FALSE(value.is<double>());
    REQUIRE_FALSE(value.is<dmn::list>());
    REQUIRE_FALSE(value.is<bool>());
    REQUIRE(value.try_as<std::string>() == std::nullopt);
    REQUIRE_THROWS_AS(value.as<double>(), std::runtime_error);
  }

  SECTION("text values convert to strings only") {
    const auto text = note->get<dmn::object>("TextValue").value();

    REQUIRE_FALSE(text.empty());
    REQUIRE(text.is<std::string>());
    REQUIRE(text.get_type() == dmn::type::text);
    REQUIRE(text.try_as<std::string>().has_value());
    REQUIRE(text.try_as<std::string>().value() == "🐶🐶🐶");
    REQUIRE(text.try_as<double>() == std::nullopt);
    REQUIRE(text.try_as<bool>() == std::nullopt);
    REQUIRE(text.as_string() == "🐶🐶🐶");
    REQUIRE_THROWS_AS(text.as<double>(), std::runtime_error);
  }

  SECTION("number values convert to multiple numbers") {
    const auto number = note->get<dmn::object>("NumericValue").value();

    REQUIRE_FALSE(number.empty());
    REQUIRE(number.is<double>());
    REQUIRE(number.get_type() == dmn::type::number);
    REQUIRE(number.try_as<double>().has_value());
    REQUIRE(number.try_as<double>().value() == 42.5);
    REQUIRE(number.try_as<int>().has_value());
    REQUIRE(number.try_as<int>().value() == 42);
    REQUIRE(number.try_as<unsigned long>().has_value());
    REQUIRE(number.try_as<unsigned long>().value() == 42UL);
    REQUIRE(number.try_as<bool>() == std::nullopt);
    REQUIRE(number.as_string() == "42.5");
    REQUIRE_THROWS_AS(number.as<std::string>(), std::runtime_error);
  }

  SECTION("boolean conversion only accepts 0 and 1") {
    const auto zero = note->get<dmn::object>("FalseValue").value();
    const auto one = note->get<dmn::object>("TrueValue").value();
    const auto number = note->get<dmn::object>("NumericValue").value();

    REQUIRE_FALSE(zero.empty());
    REQUIRE(zero.is<bool>());
    REQUIRE(zero.try_as<bool>().has_value());
    REQUIRE_FALSE(zero.try_as<bool>().value());
    REQUIRE(zero.as_string() == "0");
    REQUIRE_THROWS_AS(zero.as<std::string>(), std::runtime_error);

    REQUIRE_FALSE(one.empty());
    REQUIRE(one.is<bool>());
    REQUIRE(one.try_as<bool>().has_value());
    REQUIRE(one.try_as<bool>().value());
    REQUIRE(one.as_string() == "1");
    REQUIRE(one.try_as<std::string>() == std::nullopt);

    REQUIRE(number.try_as<bool>() == std::nullopt);
  }

  SECTION("dmn::list conversion only work for text lists") {
    const auto list_value = note->get<dmn::object>("ListValue").value();

    REQUIRE(list_value.is<dmn::list>());
    REQUIRE(list_value.try_as<dmn::list>().has_value());

    const auto list = list_value.as<dmn::list>();
    REQUIRE(list.size() == 3);
    REQUIRE(list.at(0) == "alpha");
    REQUIRE(list.at(1) == "beta");
    REQUIRE(list.at(2) == "gamma");
    REQUIRE_THROWS_AS(list.at(3), std::out_of_range);
    REQUIRE(list_value.as_string() == "alpha;beta;gamma");
    REQUIRE(list_value.try_as<std::string>() == std::nullopt);
    REQUIRE_THROWS_AS(list_value.as<double>(), std::runtime_error);
  }

  SECTION("dmn::time_date conversion only works for time date") {
    const auto date = note->get<dmn::object>("DateValue").value();

    REQUIRE(date.is<dmn::time_date>());
    REQUIRE(date.try_as<dmn::time_date>().has_value());
    REQUIRE(date.as<dmn::time_date>() == now_td);
    REQUIRE(date.as_string() == "01-02-2026 12:34:56");
    REQUIRE(date.try_as<std::string>() == std::nullopt);
    REQUIRE_THROWS_AS(date.as<double>(), std::runtime_error);
  }
}