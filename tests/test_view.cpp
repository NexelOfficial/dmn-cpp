#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <string>
#include <vector>

#include "dmn/design/view.hpp"
#include "dmn/view.hpp"
#include "dmn/note.hpp"
#include "utils.hpp"

TEST_CASE("a view can be created through the design API", "[nsf][design]") {
  auto [db, _] = utils::random_database();
  const std::string view_name = "View_" + utils::random_small_string();

  auto design_view = dmn::design::view::create(*db, view_name);
  design_view.column("#").set_title("#").set_formula("Subject");
  design_view.column("Amount").set_title("Amount").set_formula("Amount");
  design_view.set_selection_formula(dmn::formula::compile("@All"));
  design_view.save();

  const std::array<std::pair<std::string, double>, 3> expected_values{{
    {"Alpha " + utils::random_small_string(), 10.5},
    {"Beta " + utils::random_small_string(), 20.0},
    {"Gamma " + utils::random_small_string(), 30.25},
  }};

  for (const auto& [subject, amount] : expected_values) {
    auto note = db->create_note();
    note.set("Subject", subject);
    note.set("Amount", amount);
    note.save(true);
  }

  auto runtime_view = db->get_view(view_name);
  REQUIRE(runtime_view.has_value());

  auto entries = runtime_view->get_entries();
  REQUIRE(entries.size() == expected_values.size());

  std::vector<std::pair<std::string, double>> actual_values{};
  actual_values.reserve(entries.size());

  for (const auto& entry : entries) {
    REQUIRE(entry.columns.size() == 2);

    const auto subject = entry.columns.at(0).try_as<std::string>();
    const auto amount = entry.columns.at(1).try_as<double>();

    REQUIRE(subject.has_value());
    REQUIRE(amount.has_value());

    actual_values.emplace_back(*subject, *amount);
  }

  std::ranges::sort(actual_values);
  std::vector<std::pair<std::string, double>> expected_sorted(
    expected_values.begin(), expected_values.end()
  );
  std::ranges::sort(expected_sorted);

  REQUIRE(actual_values == expected_sorted);
}
