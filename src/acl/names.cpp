#include "dmn/acl/names.hpp"

#include <domino/global.h>
#include <domino/osmem.h>
#include <domino/acl.h>
#include <domino/nsf.h>

#include "dmn/misc/error.hpp"
#include "dmn/os/uhandle.hpp"
#include "dmn/os/lmbcs.hpp"
#include "dmn/os/locker.hpp"

using dmn::acl::names;

names::names() { buffer_.resize(sizeof(NAMES_LIST)); }

auto names::from_username(const std::string& name) -> names {
  dmn::dhandle_t handle = {};
  lmbcs::str converted = lmbcs::translate(name);
  dmn::status result = NSFBuildNamesList(lmbcs::cast(converted), 0, &handle);
  result.throw_if_error("Failed to build names list");

  DWORD names_size = 0;
  result = OSMemGetSize(handle, &names_size);
  result.throw_if_error("Failed to determine names size");

  auto names_obj = dmn::os::locker(handle);

  names out{};
  out.buffer_.resize(names_size);
  names_obj.read(out.buffer_.data(), out.buffer_.size());

  return out;
}

void names::set_authentication(authentication_state state) {
  auto* hdr = reinterpret_cast<NAMES_LIST*>(buffer_.data());
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

  auto current = lmbcs::view{buffer_.data(), buffer_.size()}.substr(sizeof(NAMES_LIST));

  for (size_t i = 0; i < index; ++i) {
    const auto nul = current.find(static_cast<unsigned char>(0));
    if (nul == lmbcs::view::npos) {
      return std::nullopt;
    }

    current = current.substr(nul + 1);
  }

  const auto nul = current.find(static_cast<unsigned char>(0));
  if (nul == lmbcs::view::npos) {
    return std::nullopt;
  }

  return lmbcs::translate(current.substr(0, nul));
}

auto names::get_count() const -> size_t {
  return reinterpret_cast<const NAMES_LIST*>(buffer_.data())->NumNames;
}

auto names::buffer() -> std::vector<uint8_t>& { return buffer_; }

auto names::buffer() const -> const std::vector<uint8_t>& { return buffer_; }