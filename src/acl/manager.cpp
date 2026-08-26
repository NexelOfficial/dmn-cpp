#include "dmn/acl/manager.hpp"

#include <domino/global.h>
#include <domino/acl.h>
#include <domino/osmem.h>
#include <domino/nsfdb.h>
#include <domino/miscerr.h>

#include "dmn/detail/uhandle.hpp"
#include "dmn/detail/locker.hpp"
#include "dmn/acl/names.hpp"
#include "dmn/database.hpp"
#include "dmn/error.hpp"

using dmn::acl::manager;

manager::manager(dmn::database db, detail::dhandle_t handle)
    : hdl_(handle, OSMemFree), db_(std::move(db)) {}

auto manager::read(const dmn::database& db) -> manager {
  detail::dhandle_t handle = {};
  const dmn::status result = NSFDbReadACL(db.get_handle(), &handle);
  result.throw_if_error("Failed to read ACL");

  return {db, handle};
}

auto manager::lookup_access(dmn::acl::names& names) const -> uint16_t {
  ACL_PRIVILEGES privileges = {};
  detail::dhandle_t privilege_names = {};
  uint16_t flags = 0;
  uint16_t level = 0;

  auto* names_ptr = reinterpret_cast<NAMES_LIST*>(names.buffer().data());
  const dmn::status result =
    ACLLookupAccess(hdl_.get(), names_ptr, &level, &privileges, &flags, &privilege_names);
  result.throw_if_error("Failed to look up access");

  // Wrap names in locker for freeing
  auto obj = detail::locker(privilege_names);

  if (level < ACL_LEVEL_NOACCESS || level > ACL_LEVEL_HIGHEST) {
    throw dmn::runtime_error("Access level is invalid");
  }

  return level;
}