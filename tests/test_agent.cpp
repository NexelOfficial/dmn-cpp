#include <catch2/catch_test_macros.hpp>

#include "dmn/design/agent.hpp"
#include "dmn/formula.hpp"
#include "dmn/agent.hpp"
#include "dmn/dql.hpp"
#include "utils.hpp"

namespace design = dmn::design;

TEST_CASE("Formula agent can be created and ran", "[nsf]") {
  auto [db, _] = utils::random_database();

  const std::string agent_name = "Agent_" + utils::random_small_string();
  auto design_agent = design::agent::create(*db, agent_name);

  auto code = dmn::formula{"@All; FIELD Subject := \"Modified\" + Subject"};
  design_agent.set_code(std::move(code));
  design_agent.set_trigger(design::trigger::docupdate);

  REQUIRE(design_agent.get_title() == agent_name);
  REQUIRE(design_agent.get_comment().empty());
  REQUIRE_NOTHROW(design_agent.save());

  for (size_t i = 0; i < 4; i++) {
    auto note = db->create_note();
    note.set("Subject", utils::random_small_string());
    note.set("Form", "FooBar");
    note.save(true);
  }

  auto expr = dmn::dql::eq("Form", "FooBar");
  REQUIRE(db->run_query(expr).size() == 4);

  auto agent = db->get_agent(agent_name);
  REQUIRE(agent.has_value());

  auto output = agent->run(std::nullopt);
  REQUIRE(output.has_value());
  REQUIRE(output->empty());

  auto notes = db->run_query(expr);
  REQUIRE(notes.size() == 4);

  for (const auto& note : notes) {
    auto subject = note.get<std::string>("Subject");
    REQUIRE(subject.has_value());
    REQUIRE(subject->starts_with("Modified"));
  }
}