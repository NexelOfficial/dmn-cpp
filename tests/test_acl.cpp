#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <ranges>
#include <string>
#include <vector>

#include "dmn/acl/access.hpp"
#include "dmn/acl/entry.hpp"
#include "dmn/acl/manager.hpp"
#include "dmn/acl/names.hpp"
#include "dmn/database.hpp"
#include "dmn/error.hpp"
#include "utils.hpp"

namespace acl = dmn::acl;

TEST_CASE("ACL names exposes names, authentication, and handle operations", "[acl][nsf]") {
  const std::string username = "CN=Names Test User/O=dmn";
  auto names = acl::names::from_username(username);

  REQUIRE(names.get_handle());
  REQUIRE(names.size() > 0);
  REQUIRE(names.at(0) == username);
  REQUIRE(names.try_at(0) == username);
  REQUIRE_FALSE(names.try_at(names.size()).has_value());
  REQUIRE_THROWS_AS(names.at(names.size()), dmn::out_of_range);

  std::vector<std::string> iterated;
  auto iterator = acl::names::const_iterator{&names, 0};
  const auto end = acl::names::const_iterator{&names, names.size()};
  while (iterator != end) {
    iterated.push_back(*iterator++);
  }

  REQUIRE(iterated.size() == names.size());
  for (size_t i = 0; i < names.size(); ++i) {
    REQUIRE(iterated.at(i) == names.at(i));
  }

  size_t transformed_count = 0;
  for (const auto length :
       names | std::views::filter([](const std::string& name) { return !name.empty(); }) |
         std::views::transform([](const std::string& name) { return name.size(); })) {
    REQUIRE(length > 0);
    ++transformed_count;
  }
  REQUIRE(transformed_count == names.size());

  auto prefix = acl::names::const_iterator{&names, 0};
  REQUIRE(&++prefix == &prefix);
  if (names.size() > 1) {
    REQUIRE(*prefix == names.at(1));
  }

  const std::array authentication_states{
    acl::authentication_state::unauthenticated,
    acl::authentication_state::password,
    acl::authentication_state::notes,
    acl::authentication_state::admin,
  };
  for (const auto state : authentication_states) {
    REQUIRE_NOTHROW(names.set_authentication(state));
  }

  auto clone = names.clone();
  REQUIRE(clone.get_handle());
  REQUIRE(clone.get_handle() != names.get_handle());
  REQUIRE(clone.size() == names.size());
  for (size_t i = 0; i < names.size(); ++i) {
    REQUIRE(clone.at(i) == names.at(i));
  }

  const auto released = clone.release();
  REQUIRE(released);
  REQUIRE_THROWS_AS(clone.get_handle(), dmn::invalid_handle);
  const auto released_owner = dmn::detail::locker{released, dmn::detail::ownership::free};

  utils::run_threaded([&] {
    REQUIRE_THROWS_AS(names.get_handle(), dmn::thread_access_error);
    REQUIRE_THROWS_AS(names.size(), dmn::thread_access_error);
  });
}

TEST_CASE("ACL manager persists entries, roles, and policy explicitly", "[acl][nsf]") {
  auto [db, _] = utils::random_database();
  auto control = db->create_acl();
  auto roles = control.roles();
  roles.insert(acl::role{"Approver"});

  const auto entry_access = acl::access{acl::level::author, acl::role{"[Approver]"}};
  auto added =
    control.add_entry("CN=ACL Test User/O=dmn", entry_access, acl::principal_type::person);

  added.set_name("CN=Renamed ACL Test User/O=dmn");

  const auto in_memory_entries = control.entries();
  const auto in_memory = std::ranges::find_if(in_memory_entries, [](const auto& entry) {
    return entry.get_name() == "CN=Renamed ACL Test User/O=dmn";
  });

  REQUIRE(in_memory != in_memory_entries.end());
  REQUIRE(in_memory->get_access() == entry_access);
  REQUIRE(in_memory->get_type() == acl::principal_type::person);
  REQUIRE(roles.at(0) == acl::role{"Approver"});
  REQUIRE(roles.size() == 1);
  REQUIRE(!control.admin_server().empty());
  REQUIRE_NOTHROW(control.save());

  auto stored = db->get_acl();
  REQUIRE(stored.roles().at(0) == acl::role{"Approver"});

  auto stored_entries = stored.entries();
  auto persisted = std::ranges::find_if(stored_entries, [](const auto& entry) {
    return entry.get_name() == "CN=Renamed ACL Test User/O=dmn";
  });
  REQUIRE(persisted != stored_entries.end());
  REQUIRE(persisted->get_access() == entry_access);
  REQUIRE(persisted->get_type() == acl::principal_type::person);
  REQUIRE_NOTHROW(persisted->remove());

  auto names = acl::names::from_username(persisted->get_name());
  auto lookup = db->get_access(names);
  REQUIRE(persisted->get_access() == lookup);
  REQUIRE_NOTHROW(stored.save());

  auto db_with_access = dmn::database::open(db->get_path(), std::move(names));
  REQUIRE(db_with_access.has_value());
  REQUIRE(db_with_access->get_path() == db->get_path());

  utils::run_threaded([&] {
    REQUIRE_THROWS_AS(db_with_access->get_handle(), dmn::thread_access_error);
    REQUIRE_THROWS_AS(control.get_handle(), dmn::thread_access_error);
    REQUIRE_THROWS_AS(names.get_handle(), dmn::thread_access_error);
    REQUIRE_THROWS_AS(roles.at(0), dmn::thread_access_error);
  });
}
