#pragma once

#include <memory>
#include <string>
#include <optional>
#include <vector>

#include "dmn/acl/names.hpp"
#include "dmn/detail/uhandle.hpp"
#include "dmn/dql.hpp"
#include "dmn/unid.hpp"

namespace dmn::acl {
class manager;
class access;
}  // namespace dmn::acl

namespace dmn {
class view;
class agent;
class note;

constexpr static size_t DEFAULT_QUERY_AMT = 0xF;

class database {
 public:
  using handle_t = detail::dhandle_t;
  database() = delete;

  /// Create a database.
  ///
  /// \param file New database path.
  /// \return Opened database if it was created succesfully; otherwise an empty result indicating
  /// the database does not exist.
  /// \throws dmn::native_error If the database cannot be created or opened.
  static auto create(std::string_view file) -> std::optional<database>;

  /// Delete a database.
  ///
  /// \param file Database path.
  /// \throws dmn::native_error If the database cannot be deleted.
  static void remove(std::string_view file);

  /// Open a database.
  ///
  /// \param file Database path.
  /// \return Opened database if found; otherwise an empty result indicating the database does not
  /// exist.
  /// \throws dmn::native_error If the database cannot be opened.
  static auto open(std::string_view file) -> std::optional<database>;

  /// Open a database using the specified names list for access evaluation.
  ///
  /// \param file Database path.
  /// \param names Names list used when opening the database.
  /// \return Opened database if found; otherwise an empty result indicating the database does not
  /// exist.
  /// \throws dmn::native_error If the database cannot be opened.
  static auto open(std::string_view file, const dmn::acl::names& names) -> std::optional<database>;

  /// Read this databases ACL.
  ///
  /// Changes are saved only when `dmn::acl::manager::save()` is called.
  [[nodiscard]] auto get_acl() const -> dmn::acl::manager;

  /// Create a new ACL for this database.
  ///
  /// Changes are saved only when `dmn::acl::manager::save()` is called.
  [[nodiscard]] auto create_acl() const -> dmn::acl::manager;

  /// Get the full effective ACL result for the specified names list.
  [[nodiscard]] auto get_access(const dmn::acl::names& names) const -> dmn::acl::access;

  /// Run a DQL query for the database.
  ///
  /// \param query Instance of `dmn::dql::expression` to run.
  /// \param limit Maximum amount of notes to query. Defaults to 15.
  /// \return List of `dmn::note` that match the query.
  /// \throws dmn::native_error If running the query failed.
  /// \throws dmn::invalid_handle If the underlying database handle is empty.
  [[nodiscard]] auto run_query(
    const dmn::dql::expression& query, size_t limit = DEFAULT_QUERY_AMT
  ) const -> std::vector<dmn::note>;

  /// Open a view by name.
  ///
  /// \param view_name Name of the view.
  /// \return Opened view if found; otherwise an empty result indicating the view does not exist.
  /// \throws dmn::invalid_handle If the underlying database handle is empty.
  [[nodiscard]] auto get_view(std::string_view view_name) const -> std::optional<view>;

  /// Create a new note in the database.
  ///
  /// \return Newly created note
  /// \throws dmn::invalid_handle If the underlying database handle is empty.
  /// \note The note only exists in memory after creation and doesn't have valid identifiers. Use
  /// `dmn::note::save()` to write it to disk and allocate identifiers.
  [[nodiscard]] auto create_note() const -> note;

  /// Open a note by Note ID.
  ///
  /// \param noteid Note identifier.
  /// \return Opened note if found; otherwise an empty result indicating the note does not exist.
  /// \throws dmn::invalid_handle If the underlying database handle is empty.
  [[nodiscard]] auto get_note(dmn::note_id noteid) const -> std::optional<note>;

  /// Open a note by Universal ID.
  ///
  /// \param unid Universal identifier.
  /// \return Opened note if found; otherwise an empty result indicating the note does not exist.
  /// \throws dmn::invalid_handle If the underlying database handle is empty.
  [[nodiscard]] auto get_note(dmn::unid unid) const -> std::optional<note>;

  /// Open an agent by name from the database.
  ///
  /// \param name Name of the agent to open.
  /// \return Opened agent if found; otherwise an empty result indicating the agent does not exist.
  /// \throws dmn::native_error If the agent cannot be located or opened.
  /// \throws dmn::invalid_handle If the underlying database handle is empty.
  [[nodiscard]] auto get_agent(std::string_view name) const -> std::optional<agent>;

  /// Get the database path.
  ///
  /// \return Database path using forward slash separators.
  /// \throws dmn::native_error If the database path cannot be retrieved.
  /// \throws dmn::invalid_handle If the underlying database handle is empty.
  [[nodiscard]] auto get_path() const -> std::string;

  [[nodiscard]] auto try_get_handle() const noexcept -> std::optional<handle_t> {
    return hdl_->try_get();
  }
  [[nodiscard]] auto get_handle() const -> handle_t { return hdl_ ? hdl_->get() : handle_t{}; }

 private:
  using managed_handle_t = detail::uhandle<handle_t>;
  std::shared_ptr<managed_handle_t> hdl_;

  database(handle_t handle);
};
}  // namespace dmn