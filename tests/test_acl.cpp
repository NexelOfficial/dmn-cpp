#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string>

#include "dmn/acl/access.hpp"
#include "dmn/acl/entry.hpp"
#include "dmn/acl/manager.hpp"
#include "dmn/acl/names.hpp"
#include "dmn/database.hpp"
#include "utils.hpp"

namespace acl = dmn::acl;

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
  control.save();

  auto stored = db->get_acl();
  REQUIRE(stored.roles().at(0) == acl::role{"Approver"});

  auto stored_entries = stored.entries();
  auto persisted = std::ranges::find_if(stored_entries, [](const auto& entry) {
    return entry.get_name() == "CN=Renamed ACL Test User/O=dmn";
  });

  auto names = acl::names::from_username(persisted->get_name());
  auto lookup = db->get_access(names);

  REQUIRE(persisted != stored_entries.end());
  REQUIRE(persisted->get_access() == entry_access);
  REQUIRE(persisted->get_access() == lookup);
  REQUIRE(persisted->get_type() == acl::principal_type::person);
  REQUIRE(persisted->remove());
  stored.save();
}