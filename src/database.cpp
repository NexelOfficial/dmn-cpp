#include "dmn/database.hpp"

#include <domino/global.h>
#include <domino/nsfdb.h>
#include <domino/dbmisc.h>
#include <domino/idtable.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <optional>

#include "dmn/detail/locker.hpp"
#include "dmn/detail/thread_context.hpp"
#include "dmn/detail/uhandle.hpp"
#include "dmn/acl/manager.hpp"
#include "dmn/acl/access.hpp"
#include "dmn/acl/names.hpp"
#include "dmn/lmbcs.hpp"
#include "dmn/error.hpp"
#include "dmn/agent.hpp"
#include "dmn/note.hpp"
#include "dmn/view.hpp"

using dmn::database;

static_assert(sizeof(database::handle_t) == sizeof(DBHANDLE));

constexpr size_t MAX_DQL_ENTRIES = 0xffff;

auto database::open_impl() const -> std::optional<handle_t> {
  auto& store = detail::thread_context::current().get<dmn::database>();
  store.remove_stale();

  const auto names_hdl = state_->names ? state_->names->get_handle() : detail::dhandle_t{};

  handle_t handle{};
  const dmn::status result =
    NSFDbOpenExtended(state_->path.c_str(), 0, names_hdl, nullptr, &handle, nullptr, nullptr);
  if (result.is_not_found()) {
    return std::nullopt;
  }
  result.throw_if_error("Failed to open database");

  detail::uhandle<handle_t> managed{handle, NSFDbClose};
  return store.insert(state_, std::move(managed)).handle.get();
}

auto database::create(std::string_view file) -> std::optional<database> {
  const auto converted = dmn::lmbcs::from_string(file);
  const dmn::status result = NSFDbCreate(converted.c_str(), DBCLASS_NOTEFILE, FALSE);
  result.throw_if_error("Failed to create database");
  return database::open(file);
}

void database::remove(std::string_view file) {
  auto& ctx = detail::thread_context::current();
  ctx.get<dmn::database>().remove_stale();

  const auto converted = dmn::lmbcs::from_string(file);
  const dmn::status result = NSFDbDelete(converted.c_str());
  result.throw_if_error("Failed to remove database");
}

auto database::open(std::string_view file, std::optional<acl::names> names)
  -> std::optional<database> {
  database db{{.path = dmn::lmbcs::from_string(file), .names = std::move(names)}};
  if (!db.open_impl()) {
    return std::nullopt;
  }

  return {std::move(db)};
}

auto database::get_acl() const -> dmn::acl::manager { return dmn::acl::manager::read(*this); }

auto database::create_acl() const -> dmn::acl::manager { return dmn::acl::manager::create(*this); }

auto database::get_access(const dmn::acl::names& names) const -> dmn::acl::access {
  if (names.get_count() == 0) {
    throw dmn::invalid_argument("Names list is empty");
  }

  return get_acl().lookup_access(names);
}

auto database::run_query(const dmn::dql::expression& query, size_t limit) const
  -> std::vector<dmn::note> {
  const auto input = dmn::dql::render(query);
  auto converted = dmn::lmbcs::from_string(input);

  detail::dhandle_t table_hdl = {};
  MEMHANDLE error_hdl = {};
  MEMHANDLE explain_hdl = {};
  const dmn::status result = NSFQueryDBExt2(
    get_handle(), converted.data(), converted.size(), 0, 0, 0, 0, &table_hdl, &error_hdl,
    &explain_hdl, NULLMEMHANDLE
  );
  result.throw_if_error("Failed to run DQL query");

  const detail::locker obj(table_hdl, std::numeric_limits<size_t>::max(), detail::ownership::free);

  std::vector<dmn::note> output = {};
  BOOL is_first_note = TRUE;
  DWORD note_id = 0;

  const size_t max_entries = std::min(MAX_DQL_ENTRIES, limit);
  while (IDScan(table_hdl, is_first_note, &note_id) == TRUE && output.size() < max_entries) {
    is_first_note = FALSE;

    auto note = get_note(note_id);
    if (note) {
      output.emplace_back(*note);
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
  dmn::lmbcs raw_path;
  raw_path.resize(MAXPATH);

  const dmn::status result = NSFDbPathGet(get_handle(), raw_path.data(), nullptr);
  result.throw_if_error("Failed to get database path");

  raw_path.resize(strlen(raw_path.c_str()));

  // Make path HTTP safe
  std::ranges::transform(raw_path, raw_path.begin(), [](auto& c) { return c == '\\' ? '/' : c; });
  return raw_path.to_string();
}

auto database::get_handle() const -> handle_t {
  auto& store = detail::thread_context::current().get<dmn::database>();
  store.remove_stale();

  auto* entry = store.find(state_);
  if (entry != nullptr) {
    return entry->handle.get();
  }

  auto handle = open_impl();
  if (!handle) {
    throw dmn::invalid_handle("The database has disappeared");
  }
  return *handle;
}