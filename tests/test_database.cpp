#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string>

#include "dmn/database.hpp"
#include "dmn/dql.hpp"
#include "utils.hpp"

TEST_CASE("a note can be persisted and reopened", "[nsf][database]") {
  auto [db, _] = utils::random_database();

  const std::string subject = "Database test subject " + utils::random_small_string();

  utils::note_guard note{db->create_note()};
  note->set("Subject", subject);
  note->append("Category", "database");
  note->save(true);

  const auto noteid = note->info<dmn::info::note_id>();
  const auto unid = note->info<dmn::info::unid>();

  REQUIRE(noteid.value != 0);
  REQUIRE(unid.to_string().size() == 32);

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

TEST_CASE("a persisted note can be found with DQL", "[nsf][database]") {
  auto [db, _] = utils::random_database();

  const std::string subject = "DQL test subject " + utils::random_small_string();

  utils::note_guard note{db->create_note()};
  note->set("Subject", subject);
  note->save(true);

  const auto noteid = note->info<dmn::info::note_id>();
  const auto results = db->run_query(dmn::dql::eq("Subject", subject), 10);

  REQUIRE(std::ranges::any_of(results, [&](const dmn::note& item) {
    return item.info<dmn::info::note_id>() == noteid;
  }));
}