#include <catch2/catch_test_macros.hpp>

#include "dmn/design/agent.hpp"
#include "dmn/lotusscript.hpp"
#include "dmn/formula.hpp"
#include "dmn/agent.hpp"
#include "dmn/dql.hpp"
#include "utils.hpp"

namespace design = dmn::design;

constexpr std::string_view FORMULA_CODE = R"(
@All; FIELD Subject := "Modified" + Subject
)";

constexpr std::string_view LOTUSSCRIPT_CODE = R"(
Sub Initialize
    Dim session As New NotesSession
    Dim db As NotesDatabase
    Dim collection As NotesDocumentCollection
    Dim doc As NotesDocument

    Set db = session.CurrentDatabase
    Set collection = db.AllDocuments

    Set doc = collection.GetFirstDocument()
    While Not doc Is Nothing
        doc.Subject = "Modified" & doc.Subject(0)
        Call doc.Save(True, False)
        Set doc = collection.GetNextDocument(doc)
    Wend
End Sub
)";

namespace {
auto setup_modify_agent(const dmn::database& db) -> design::agent {
  const std::string title = "Agent_" + utils::random_small_string();
  auto design_agent = design::agent::create(db, title);
  design_agent.set_trigger(design::trigger::docupdate);

  REQUIRE(design_agent.get_title() == title);
  REQUIRE(design_agent.get_comment().empty());
  REQUIRE_NOTHROW(design_agent.save());
  return design_agent;
}

void check_modify_agent(const dmn::database& db, std::string_view title) {
  for (size_t i = 0; i < 4; i++) {
    auto note = db.create_note();
    note.set("Subject", utils::random_small_string());
    note.set("Form", "FooBar");
    note.save(true);
  }

  auto expr = dmn::dql::eq("Form", "FooBar");
  REQUIRE(db.run_query(expr).size() == 4);

  auto agent = db.get_agent(title);
  REQUIRE(agent.has_value());

  auto output = agent->run(std::nullopt);
  REQUIRE(output.has_value());
  REQUIRE(output->empty());

  auto notes = db.run_query(expr);
  REQUIRE(notes.size() == 4);

  for (const auto& note : notes) {
    auto subject = note.get<std::string>("Subject");
    REQUIRE(subject.has_value());
    REQUIRE(subject->starts_with("Modified"));
  }
}
}  // namespace

TEST_CASE("Formula agent can be created and ran", "[nsf]") {
  auto [db, _] = utils::random_database();
  auto design_agent = setup_modify_agent(*db);

  auto code = dmn::formula{FORMULA_CODE};
  design_agent.set_code(std::move(code));
  REQUIRE_NOTHROW(design_agent.save());

  check_modify_agent(*db, design_agent.get_title());
}

TEST_CASE("LotusScript agent can be created and ran", "[nsf]") {
  auto [db, _] = utils::random_database();
  auto design_agent = setup_modify_agent(*db);

  auto code = dmn::lotusscript{LOTUSSCRIPT_CODE};
  design_agent.set_code(std::move(code));
  REQUIRE_NOTHROW(design_agent.save());

  check_modify_agent(*db, design_agent.get_title());
}