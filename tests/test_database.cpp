#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string>
#include <vector>

#include "dmn/database.hpp"
#include "dmn/note.hpp"
#include "dmn/dql.hpp"
#include "utils.hpp"

TEST_CASE("a note can be persisted and reopened", "[nsf][database]") {
  auto [db, _] = utils::random_database();

  const std::string subject = "Database test subject " + utils::random_small_string();

  auto note = db->create_note();
  note.set("Subject", subject);
  note.set("Category", "database");
  note.save(true);

  const auto noteid = note.info<dmn::info::note_id>();
  const auto unid = note.info<dmn::info::unid>();

  REQUIRE(noteid.value != 0);
  REQUIRE(unid.to_string().size() == 32);

  auto non_existent = dmn::database::open("Empty.nsf");
  REQUIRE_FALSE(non_existent.has_value());

  SECTION("reopen by note ID") {
    const auto reopened = db->get_note(noteid);

    REQUIRE(reopened.has_value());
    REQUIRE(reopened->info<dmn::info::note_id>() == noteid);
    REQUIRE(reopened->info<dmn::info::unid>() == unid);

    const auto stored_subject = reopened->get<std::string>("Subject");

    REQUIRE(stored_subject.has_value());
    REQUIRE(*stored_subject == subject);
  }

  SECTION("reopen by UNID") {
    const auto reopened = db->get_note(unid);

    REQUIRE(reopened.has_value());
    REQUIRE(reopened->info<dmn::info::note_id>() == noteid);
    REQUIRE(reopened->info<dmn::info::unid>() == unid);
  }
}

TEST_CASE("a database can be used in threads", "[nsf][database]") {
  auto [db, _] = utils::random_database();

  const auto main_handle = db->get_handle();
  REQUIRE(main_handle);

  utils::run_threaded([&]() {
    const auto handle = db->get_handle();
    REQUIRE(handle);
    REQUIRE(db->get_handle() == handle);

    const auto copy = *db;
    REQUIRE(copy.get_handle() == handle);

    const auto note = db->create_note();
    note.set("Foo", "Bar");
    note.save(true);

    const auto noteid = note.info<dmn::info::note_id>();

    const auto reopened = db->get_note(noteid);
    REQUIRE(reopened);
    REQUIRE(reopened->get_handle());
    REQUIRE(reopened->info<dmn::info::note_id>() == noteid);
  });

  REQUIRE(db->get_handle() == main_handle);
}

TEST_CASE("a persisted note can be found with DQL", "[nsf][database]") {
  auto [db, _] = utils::random_database();

  const std::string subject = "DQL test subject " + utils::random_small_string();

  auto note = db->create_note();
  note.set("Subject", subject);
  note.save(true);

  const auto noteid = note.info<dmn::info::note_id>();
  const auto results = db->run_query(dmn::dql::eq("Subject", subject), 10);

  REQUIRE(std::ranges::any_of(results, [&](const dmn::note& item) {
    return item.info<dmn::info::note_id>() == noteid;
  }));
}

TEST_CASE("DQL expressions render supported values and operators", "[dql]") {
  using dmn::dql;

  REQUIRE(dql::render(dql::eq("Subject", "O'Brien\n")) == "Subject = 'O''Brien\\n'");
  REQUIRE(dql::render(dql::ne("Count", 2)) == "not Count = 2");
  REQUIRE(dql::render(dql::lt("Count", 2L)) == "Count < 2");
  REQUIRE(dql::render(dql::lte("Count", 2LL)) == "Count <= 2");
  REQUIRE(dql::render(dql::gt("Amount", 1.5)) == "Amount > 1.5");
  REQUIRE(dql::render(dql::gte("Enabled", true)) == "Enabled >= true");
  REQUIRE(dql::render(dql::contains("Subject", "needle")) == "Subject contains ('needle')");
  REQUIRE(
    dql::render(dql::eq("Created", dql::datetime("2026-10-21"))) ==
    "Created = @dt('2026-10-21')"
  );

  const auto grouped = dql::all({
    dql::eq("Form", std::string{"Memo"}),
    dql::any({dql::gt("Score", 10), dql::eq("Approved", false)}),
  });
  REQUIRE(dql::render(grouped) == "Form = 'Memo' and (Score > 10 or Approved = false)");

  REQUIRE_THROWS_AS(dql::eq("invalid field", 1), dmn::invalid_argument);
  REQUIRE_THROWS_AS(dql::contains("Subject", ""), dmn::invalid_argument);
  REQUIRE_THROWS_AS(dql::datetime(""), dmn::invalid_argument);
  REQUIRE_THROWS_AS(dql::all({}), dmn::invalid_argument);
  REQUIRE_THROWS_AS(dql::any({}), dmn::invalid_argument);
  REQUIRE_THROWS_AS(dql::render({.mode = dql::expression::oper::raw}), dmn::invalid_argument);
}