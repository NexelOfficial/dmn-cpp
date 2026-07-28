#include "dmn/nsf/database.hpp"

#include <domino/global.h>
#include <domino/nsfdb.h>
#include <domino/dbmisc.h>
#include <domino/idtable.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <optional>

#include "dmn/addin/session.hpp"
#include "dmn/acl/manager.hpp"
#include "dmn/acl/names.hpp"
#include "dmn/nos/agent.hpp"
#include "dmn/misc/error.hpp"
#include "dmn/os/lmbcs.hpp"
#include "dmn/os/locker.hpp"
#include "dmn/nsf/note.hpp"
#include "dmn/nsf/view.hpp"

using dmn::database;

static_assert(sizeof(database::handle_t) == sizeof(DBHANDLE));

constexpr size_t MAX_DQL_ENTRIES = 0xffff;

database::database(handle_t handle)
    : hdl_(std::make_shared<managed_handle_t>(handle, NSFDbClose)) {}

auto database::open(std::string_view file) -> std::optional<database> {
  return database::open(file, {});
}

auto database::open(std::string_view file, const dmn::acl::names& names)
  -> std::optional<database> {
  (void)dmn::session::instance();

  // Allocate memory on server for names
  using locker = dmn::os::locker;
  const auto names_obj =
    locker::allocate(names.get_count() == 0 ? std::vector<uint8_t>{} : names.buffer());
  const dmn::dhandle_t names_hdl = names_obj ? names_obj->get_handle() : NULLHANDLE;

  handle_t handle = {};
  const lmbcs::str converted = lmbcs::translate(file);
  const dmn::status result =
    NSFDbOpenExtended(lmbcs::cast(converted), 0, names_hdl, nullptr, &handle, nullptr, nullptr);

  if (result.is_not_found()) {
    return std::nullopt;
  }
  result.throw_if_error("Failed to open database");

  return database(handle);
}

auto database::get_access_level(dmn::acl::names& names) const -> uint16_t {
  if (names.get_count() == 0) {
    throw std::invalid_argument("Names list is empty");
  }

  const auto acl = dmn::acl::manager::read(*this);
  return acl.lookup_access(names);
}

auto database::run_query(const dmn::dql::expression& query, size_t limit) const
  -> std::vector<dmn::note> {
  const auto input = dmn::dql::render(query);
  auto converted = lmbcs::translate(input);

  dmn::dhandle_t table_hdl = {};
  MEMHANDLE error_hdl = {};
  MEMHANDLE explain_hdl = {};
  const dmn::status result = NSFQueryDBExt2(
    get_handle(), lmbcs::cast(converted), converted.size(), 0, 0, 0, 0, &table_hdl, &error_hdl,
    &explain_hdl, NULLMEMHANDLE
  );
  result.throw_if_error("Failed to run DQL query");

  const dmn::os::locker obj(
    table_hdl, std::numeric_limits<size_t>::max(), dmn::os::ownership::free
  );

  std::vector<dmn::note> output = {};
  BOOL is_first_note = TRUE;
  DWORD note_id = 0;

  const size_t max_entries = std::min(MAX_DQL_ENTRIES, limit);
  while (IDScan(table_hdl, is_first_note, &note_id) == TRUE && output.size() < max_entries) {
    is_first_note = FALSE;

    auto note = get_note(note_id);
    if (note) {
      output.emplace_back(std::move(*note));
    }
  }

  return output;
}

auto database::get_view(std::string_view view_name) const -> std::optional<dmn::view> {
  return dmn::view::open(*this, view_name);
};

auto database::create_note() const -> dmn::note { return dmn::note::create(*this); }

auto database::get_note(dmn::note_id noteid) const -> std::optional<dmn::note> {
  return dmn::note::open(*this, noteid);
};

auto database::get_note(dmn::unid unid) const -> std::optional<dmn::note> {
  return dmn::note::open(*this, unid);
}

auto database::get_agent(std::string_view name) const -> std::optional<dmn::agent> {
  return dmn::agent::open(*this, name);
}

auto database::get_path() const -> std::string {
  lmbcs::str raw_path(MAXPATH, '\0');
  const dmn::status result = NSFDbPathGet(get_handle(), lmbcs::cast(raw_path), nullptr);
  result.throw_if_error("Failed to get database path");

  raw_path.resize(strlen(lmbcs::cast(raw_path)));

  // Make path HTTP safe
  std::ranges::transform(raw_path, raw_path.begin(), [](auto& c) { return c == '\\' ? '/' : c; });

  return lmbcs::translate(raw_path);
}
