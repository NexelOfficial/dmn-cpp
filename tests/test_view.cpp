#include <catch2/catch_test_macros.hpp>

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
  design_view.column("Subject")
    .set_title("Document Subject")
    .set_formula(dmn::formula{"Subject"})
    .set_header_font(font_c)
    .set_item_font(font_b)
    .set_sorting(design::column::sorting::descending, true);

  design_view.column("#")
    .set_formula(dmn::formula{"@Text(@DocNumber)"})
    .set_item_font(font_c)
    .set_header_font(font_a);

  design_view.column("Amount")
    .set_formula(dmn::formula{"Amount"})
    .set_item_font(font_a)
    .set_header_font(font_b);

  design_view.column("Combi")
    .set_formula(dmn::formula{"Subject + \" = #\" + @Text(Amount)"})
    .set_item_name(utils::random_small_string());

  design_view.set_selection_formula(dmn::formula{"@All"});
  design_view.set_background_color(design::color::light_gray);
  design_view.save();

  const std::array<std::pair<std::string, double>, 4> values{{
    {"Alpha", 10.5},
    {"Alpha", 20},
    {"Beta " + utils::random_small_string(), 30.25},
    {"Gamma " + utils::random_small_string(), 60},
  }};

  for (const auto& [subject, amount] : values) {
    auto note = db->create_note();
    note.set("Subject", subject);
    note.set("Amount", amount);
    note.save(true);
  }

  auto runtime_view = db->get_view(view_name);
  REQUIRE(runtime_view.has_value());

  auto entries = runtime_view->get_entries({.key = "Alpha"});
  REQUIRE(entries.size() == 2);

  for (const auto& entry : entries) {
    REQUIRE(entry.columns.size() == 4);
    REQUIRE(entry.columns.at(1).is<std::string>());
    REQUIRE(entry.columns.at(2).is<double>());
    REQUIRE(entry.columns.at(3).is<std::string>());

    const auto nr = entry.columns.at(1).try_as<std::string>();
    const auto amount = entry.columns.at(2).as_string();
    const auto combi = entry.columns.at(3).try_as<std::string>();

    REQUIRE(nr.has_value());
    REQUIRE(amount.has_value());
    REQUIRE(combi.has_value());
    REQUIRE("Alpha = #" + *amount == combi);
  }
}
