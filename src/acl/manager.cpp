#include "dmn/acl/manager.hpp"

#include <domino/global.h>
#include <domino/acl.h>
#include <domino/dname.h>
#include <domino/names.h>
#include <domino/nsfdb.h>
#include <domino/osmem.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <exception>
#include <utility>

#include "dmn/acl/entry.hpp"
#include "dmn/acl/role.hpp"
#include "dmn/database.hpp"
#include "dmn/error.hpp"
#include "dmn/lmbcs.hpp"

using dmn::acl::manager;

namespace {
constexpr auto entry_flag_mask = dmn::acl::flags(
  dmn::acl::flag::author_no_create, dmn::acl::flag::no_delete,
  dmn::acl::flag::create_personal_agent, dmn::acl::flag::create_personal_folder,
  dmn::acl::flag::create_folder, dmn::acl::flag::create_lotusscript, dmn::acl::flag::public_reader,
  dmn::acl::flag::public_writer, dmn::acl::flag::monitors_disallowed, dmn::acl::flag::no_replicate
);
constexpr uint16_t principal_type_mask = static_cast<uint16_t>(dmn::acl::principal_type::server) |
                                         static_cast<uint16_t>(dmn::acl::principal_type::person) |
                                         static_cast<uint16_t>(dmn::acl::principal_type::group);

struct enum_context {
  std::vector<dmn::acl::entry>* entries;
  dmn::acl::manager mgr;
  std::exception_ptr error;
};

[[nodiscard]] auto native_flags(const dmn::acl::entry& value) -> uint16_t {
  auto result = value.get_access().flags | static_cast<uint16_t>(value.get_type());
  if (value.is_administration_server()) {
    result |= ACL_FLAG_ADMIN_SERVER;
  }
  return result;
}

[[nodiscard]] auto roles_from_native(
  const ACL_PRIVILEGES& privileges, const dmn::acl::role_map& roles
) -> std::vector<dmn::acl::role> {
  std::vector<dmn::acl::role> result;
  for (const auto& [i, role] : roles) {
    auto has_priv = ACLIsPrivSet(privileges, i + ACL_BITPRIVCOUNT) != 0;
    if (has_priv && !role.name().empty()) {
      result.emplace_back(roles.at(i));
    }
  }
  return result;
}

[[nodiscard]] auto roles_to_native(
  const dmn::acl::manager& mgr, const std::vector<dmn::acl::role>& roles
) -> ACL_PRIVILEGES {
  const auto defined = mgr.roles();

  ACL_PRIVILEGES result{};
  size_t passed = 0;
  for (const auto& [i, role] : defined) {
    const auto it = std::ranges::find(roles, role);
    if (it != roles.end()) {
      ACLSetPriv(result, i + ACL_BITPRIVCOUNT);
      ++passed;
    }
  }

  if (passed != roles.size()) {
    throw dmn::invalid_argument("One or more ACL roles don't exist");
  }
  return result;
}

void LNCALLBACK read_entry(
  void* parameter, char* name, uint16_t access_level, ACL_PRIVILEGES* privileges,
  uint16_t access_flags
) {
  auto& context = *static_cast<enum_context*>(parameter);
  if (context.error) {
    return;
  }

  try {
    dmn::lmbcs_view view{name, std::strlen(name)};
    dmn::acl::access access{};
    access.level = static_cast<dmn::acl::level>(access_level);
    access.flags = access_flags & entry_flag_mask;
    access.roles = roles_from_native(*privileges, context.mgr.roles());

    dmn::acl::entry entry{
      context.mgr, view.to_string(), std::move(access),
      static_cast<dmn::acl::principal_type>(access_flags & principal_type_mask),
      (access_flags & ACL_FLAG_ADMIN_SERVER) != 0
    };

    context.entries->push_back(std::move(entry));
  } catch (...) {
    context.error = std::current_exception();
  }
}
}  // namespace

manager::manager(dmn::database db, dmn::detail::dhandle_t handle, origin source)
    : db_(std::move(db)),
      hdl_(std::make_shared<managed_handle_t>(handle, OSMemFree)),
      origin_(source) {}

auto manager::read(const dmn::database& db) -> manager {
  dmn::detail::uhandle<dmn::detail::dhandle_t> handle(OSMemFree);
  const dmn::status result = NSFDbReadACL(db.get_handle(), handle.data());
  result.throw_if_error("Failed to read ACL");
  return {db, handle.release(), origin::existing};
}

auto manager::create(const dmn::database& db) -> manager {
  dmn::detail::uhandle<dmn::detail::dhandle_t> handle(OSMemFree);
  const dmn::status result = ACLCreate(handle.data());
  result.throw_if_error("Failed to create ACL");
  return {db, handle.release(), origin::newly_created};
}

auto manager::lookup_access(const dmn::acl::names& names) const -> dmn::acl::access {
  ACL_PRIVILEGES privileges{};
  uint16_t access_flags = 0;
  uint16_t access_level = 0;

  auto* names_list = reinterpret_cast<NAMES_LIST*>(const_cast<std::byte*>(names.buffer().data()));
  const dmn::status result =
    ACLLookupAccess(get_handle(), names_list, &access_level, &privileges, &access_flags, nullptr);
  result.throw_if_error("Failed to look up access");

  auto access = dmn::acl::access{};
  access.level = static_cast<dmn::acl::level>(access_level);
  access.flags = access_flags & entry_flag_mask;

  auto all_roles = roles_from_native(privileges, role_map{*this});
  access.roles.reserve(all_roles.size());

  for (const auto& role : all_roles) {
    access.roles.emplace_back(role);
  }
  return access;
}

auto manager::entries() const -> std::vector<dmn::acl::entry> {
  std::vector<dmn::acl::entry> data;
  enum_context context{.entries = &data, .mgr = *this};

  const dmn::status result = ACLEnumEntries(get_handle(), read_entry, &context);
  result.throw_if_error("Failed to read ACL entries");
  if (context.error) {
    std::rethrow_exception(context.error);
  }

  return data;
}

auto manager::get_entry(std::string_view name) const -> std::optional<dmn::acl::entry> {
  auto all = entries();
  const auto found =
    std::ranges::find_if(all, [name](const auto& value) { return value.get_name() == name; });
  if (found == all.end()) {
    return std::nullopt;
  }
  return std::move(*found);
}

auto manager::add_entry(
  std::string_view name, dmn::acl::access value, dmn::acl::principal_type type
) const -> dmn::acl::entry {
  auto entry = dmn::acl::entry{*this, std::string{name}, std::move(value), type, false};
  auto converted_name = dmn::lmbcs::from_string(entry.name_);
  auto privileges = roles_to_native(*this, entry.access_.roles);

  const dmn::status result = ACLAddEntry(
    get_handle(), converted_name.c_str(), static_cast<uint16_t>(entry.access_.level), &privileges,
    native_flags(entry)
  );
  result.throw_if_error("Failed to add ACL entry");
  return entry;
}

auto manager::add_entry(std::string_view name, dmn::acl::level value) const -> dmn::acl::entry {
  return add_entry(name, dmn::acl::access{value});
}

void manager::update_entry(std::string_view name, const dmn::acl::entry& value) const {
  auto converted_name = dmn::lmbcs::from_string(name);
  auto converted_new_name = dmn::lmbcs::from_string(value.name_);
  auto privileges = roles_to_native(*this, value.access_.roles);
  const char* old_name = name.empty() ? nullptr : converted_name.c_str();
  const char* new_name = value.name_ == name ? nullptr : converted_new_name.c_str();
  uint16_t update_flags = ACL_UPDATE_LEVEL | ACL_UPDATE_PRIVILEGES | ACL_UPDATE_FLAGS;

  if (new_name != nullptr) {
    update_flags |= ACL_UPDATE_NAME;
  }

  const dmn::status result = ACLUpdateEntry(
    get_handle(), old_name, update_flags, new_name, static_cast<uint16_t>(value.access_.level),
    &privileges, native_flags(value)
  );
  result.throw_if_error("Failed to update ACL entry");
}

auto manager::roles() const -> dmn::acl::role_map { return {*this}; }

auto manager::admin_server() const -> std::string {
  dmn::lmbcs admin_server;
  admin_server.resize(MAXUSERNAME);

  const dmn::status result = ACLGetAdminServer(get_handle(), admin_server.data());
  result.throw_if_error("Failed to read ACL administration server");
  return admin_server.to_string();
}

void manager::set_admin_server(std::string_view server) const {
  auto converted = dmn::lmbcs::from_string(server);
  if (server.empty()) {
    const dmn::status result = ACLSetAdminServer(get_handle(), converted.data());
    result.throw_if_error("Failed to clear ACL administration server");
    return;
  }

  std::array<char, MAXUSERNAME> canonical{};
  uint16_t canonical_length = 0;
  dmn::status result = DNCanonicalize(
    0, nullptr, converted.c_str(), canonical.data(), canonical.size(), &canonical_length
  );
  result.throw_if_error("Failed to canonicalize ACL administration server");

  result = ACLSetAdminServer(get_handle(), canonical.data());
  result.throw_if_error("Failed to set ACL administration server");
}

void manager::save() {
  const uint16_t method = origin_ == origin::newly_created ? 1 : 0;
  const dmn::status result = NSFDbStoreACL(db_.get_handle(), get_handle(), 0, method);
  result.throw_if_error("Failed to store ACL");
  origin_ = origin::existing;
}
