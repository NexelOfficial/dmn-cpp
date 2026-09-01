#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <string>
#include <vector>

#include "dmn/design/color.hpp"
#include "dmn/design/column.hpp"
#include "dmn/design/view.hpp"
#include "dmn/view.hpp"
#include "dmn/note.hpp"
#include "utils.hpp"

namespace design = dmn::design;

TEST_CASE("a view can be created through the design API", "[nsf][design]") {
  auto [db, _] = utils::random_database();
  const std::string view_name = "View_" + utils::random_small_string();

  const design::font font_a(
    design::font::size::normal, design::color::magenta, design::font::style::bold
  );
  const design::font font_b(
    design::font::size::xlarge, design::font::style::bold, design::font::style::underline
  );
  const design::font font_c(
    design::font::size::small, design::font::style::italic, design::color::green
  );

  auto design_view = design::view::create(*db, view_name);
  design_view.column("#")
    .set_formula(dmn::formula{"\"Nr.\" + @Text(@DocNumber)"})
    .set_header_font(font_a)
    .set_sorting(design::column::sorting::descending, true);

  design_view.column("Subject")
    .set_title("Document Subject")
    .set_formula(dmn::formula{"Subject"})
    .set_header_font(font_c)
    .set_item_font(font_b)
    .set_sorting(design::column::sorting::ascending, true);

  design_view.column("Amount")
    .set_formula(dmn::formula{"Amount"})
    .set_item_font(font_a)
    .set_header_font(font_b);

  design_view.set_selection_formula(dmn::formula{"@All"});
  design_view.set_background_color(design::color::light_gray);
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
    REQUIRE(entry.columns.size() == 3);

    const auto nr = entry.columns.at(0).try_as<std::string>();
    const auto subject = entry.columns.at(1).try_as<std::string>();
    const auto amount = entry.columns.at(2).try_as<double>();

    REQUIRE(nr.has_value());
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
