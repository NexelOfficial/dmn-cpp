#include "dmn/acl/entry.hpp"

#include <utility>

#include <domino/global.h>
#include <domino/acl.h>

using dmn::acl::entry;

entry::entry(
  manager mgr, std::string name, acl::access access, principal_type type, bool administration_server
)
    : mgr_(std::move(mgr)),
      name_(std::move(name)),
      access_(std::move(access)),
      type_(type),
      administration_server_(administration_server) {}

auto entry::get_name() const noexcept -> std::string_view { return name_; }

auto entry::get_access() const noexcept -> const dmn::acl::access& { return access_; }

auto entry::get_level() const noexcept -> dmn::acl::level { return access_.level; }

auto entry::get_flags() const noexcept -> uint16_t { return access_.flags; }

auto entry::get_roles() const noexcept -> const std::vector<dmn::acl::role>& {
  return access_.roles;
}

auto entry::get_type() const noexcept -> dmn::acl::principal_type { return type_; }

auto entry::is_administration_server() const noexcept -> bool { return administration_server_; }

auto entry::set_name(std::string_view name) -> entry& {
  auto updated = *this;
  updated.name_ = name;
  mgr_.update_entry(name_, updated);
  name_ = name;
  return *this;
}

auto entry::set_access(dmn::acl::access value) -> entry& {
  auto updated = *this;
  updated.access_ = std::move(value);
  mgr_.update_entry(name_, updated);
  access_ = std::move(updated.access_);
  return *this;
}

auto entry::set_level(dmn::acl::level value) -> entry& {
  auto updated = access_;
  updated.level = value;
  return set_access(std::move(updated));
}

auto entry::set_flags(uint16_t value) -> entry& {
  auto updated = access_;
  updated.flags = value;
  return set_access(std::move(updated));
}

auto entry::set_roles(std::vector<dmn::acl::role> value) -> entry& {
  auto updated = access_;
  updated.roles = std::move(value);
  return set_access(std::move(updated));
}

auto entry::set_type(dmn::acl::principal_type value) -> entry& {
  auto updated = *this;
  updated.type_ = value;
  mgr_.update_entry(name_, updated);
  type_ = value;
  return *this;
}

auto entry::set_administration_server(bool value) -> entry& {
  auto updated = *this;
  updated.administration_server_ = value;
  mgr_.update_entry(name_, updated);
  administration_server_ = value;
  return *this;
}

auto entry::remove() -> bool {
  const auto converted = dmn::lmbcs::from_string(name_);
  return ACLDeleteEntry(mgr_.get_handle(), converted.c_str()) == NOERROR;
}