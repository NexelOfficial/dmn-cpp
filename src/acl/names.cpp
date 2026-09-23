#include "dmn/acl/names.hpp"

#include <domino/global.h>
#include <domino/osmem.h>
#include <domino/acl.h>
#include <domino/nsf.h>

#include "dmn/detail/uhandle.hpp"
#include "dmn/detail/locker.hpp"
#include "dmn/lmbcs.hpp"
#include "dmn/error.hpp"

using dmn::acl::names;

names::names(handle_t handle) : hdl_(handle, OSMemFree) {}

auto names::from_username(std::string_view name) -> names {
  detail::dhandle_t handle{};
  auto converted = dmn::lmbcs::from_string(name);
  const dmn::status result = NSFBuildNamesList(converted.data(), 0, &handle);
  result.throw_if_error("Failed to build names list");
  return {handle};
}

void names::set_authentication(authentication_state state) const {
  const auto lock = get_cursor();
  auto* hdr = lock.get_pointer<NAMES_LIST>();
  switch (state) {
    case authentication_state::unauthenticated:
      hdr->Authenticated = 0;
      break;
    case authentication_state::password:
      hdr->Authenticated = NAMES_LIST_PASSWORD_AUTHENTICATED;
      break;
    case authentication_state::notes:
      hdr->Authenticated = NAMES_LIST_AUTHENTICATED;
      break;
    case authentication_state::admin:
      hdr->Authenticated = NAMES_LIST_FULL_ADMIN_ACCESS;
      break;
  }
}

auto names::get_name(size_t index) const -> std::optional<std::string> {
  if (index >= get_count()) {
    return std::nullopt;
  }

  const auto lock = get_cursor();
  const auto* ptr = lock.get_pointer<dmn::lmbcs::char_t>();
  auto current = dmn::lmbcs_view{ptr, lock.size()}.substr(sizeof(NAMES_LIST));

  for (size_t i = 0; i < index; ++i) {
    const auto nul = current.find(dmn::lmbcs::char_t{});
    if (nul == dmn::lmbcs_view::npos) {
      return std::nullopt;
    }

    current = current.substr(nul + 1);
  }

  const auto nul = current.find(dmn::lmbcs::char_t{});
  if (nul == dmn::lmbcs_view::npos) {
    return std::nullopt;
  }

  return lmbcs_view(current.substr(0, nul)).to_string();
}

auto names::get_count() const -> size_t {
  const auto lock = get_cursor();
  return lock.get_pointer<NAMES_LIST>()->NumNames;
}