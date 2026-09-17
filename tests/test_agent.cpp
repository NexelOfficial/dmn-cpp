#include <catch2/catch_test_macros.hpp>

#include "dmn/design/agent.hpp"
#include "dmn/formula.hpp"
#include "utils.hpp"

namespace design = dmn::design;

TEST_CASE("Agent can be created", "[acl][nsf]") {
  auto [db, _] = utils::random_database();
  auto design_agent = design::agent::create(*db, utils::random_small_string());

  auto code = dmn::formula{"@All"};
  design_agent.set_code(std::move(code));
  design_agent.save();
}