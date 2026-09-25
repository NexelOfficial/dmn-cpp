#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string>

#include "dmn/detail/thread_context.hpp"
#include "dmn/database.hpp"
#include "dmn/error.hpp"
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
    REQUIRE_THROWS_AS(db->get_handle(), dmn::thread_error);

    const dmn::detail::thread_context ctx{};
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

  utils::run_threaded([&]() {
    REQUIRE_THROWS_AS(db->get_handle(), dmn::thread_error);

    const dmn::detail::thread_context ctx{};
    REQUIRE(db->get_handle());
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