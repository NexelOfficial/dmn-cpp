#include "dmn/acl/role.hpp"
#include "dmn/error.hpp"

#include <domino/global.h>
#include <domino/acl.h>

using dmn::acl::role;
using dmn::acl::role_map;

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto role_map::capacity() const noexcept -> size_t { return ACL_PRIVCOUNT - ACL_BITPRIVCOUNT; }

auto role_map::size() const -> size_t {
  size_t result = 0;
  for (size_t index = 0; index < capacity(); ++index) {
    if (try_at(index)) {
      ++result;
    }
  }

  return result;
}

auto role_map::at(size_t key) const -> role {
  if (key >= capacity()) {
    throw dmn::out_of_range("Index is outside the role list");
  }

  dmn::lmbcs value;
  value.resize(ACL_PRIVSTRINGMAX);

  const dmn::status result =
    ACLGetPrivName(mgr_.get_handle(), key + ACL_BITPRIVCOUNT, value.data<char>());
  if (result.is_not_found()) {
    throw dmn::out_of_range("Index is not found in the role list");
  }
  result.throw_if_error("Failed to read role from ACL");

  const dmn::lmbcs resized(value.data());
  return role{resized.to_string()};
}

auto role_map::try_at(size_t key) const noexcept -> std::optional<role> {
  try {
    return at(key);
  } catch (...) {
    return std::nullopt;
  }
}

void role_map::insert(const role& value) {
  dmn::lmbcs buffer;
  buffer.resize(ACL_PRIVSTRINGMAX);

  for (size_t key = 0; key < capacity(); ++key) {
    if (!try_at(key)) {
      set(key, value);
      return;
    }
  }

  throw dmn::out_of_range("Role list has reached its maximum capacity");
}

void role_map::set(size_t key, const role& value) {
  if (key >= capacity()) {
    throw dmn::out_of_range("Index is outside the role list");
  }

  auto converted = dmn::lmbcs::from_string(value.name());
  const dmn::status result =
    ACLSetPrivName(mgr_.get_handle(), key + ACL_BITPRIVCOUNT, converted.data());
  result.throw_if_error("Failed to set role in ACL");
}

void role_map::erase(size_t key) {
  if (key >= capacity()) {
    throw dmn::out_of_range("Index is outside the role list");
  }

  const dmn::status result = ACLSetPrivName(mgr_.get_handle(), key + ACL_BITPRIVCOUNT, nullptr);
  result.throw_if_error("Failed to remove role from ACL");
}

void role_map::clear() {
  for (size_t index = 0; index < capacity(); ++index) {
    if (try_at(index)) {
      erase(index);
    }
  }
}