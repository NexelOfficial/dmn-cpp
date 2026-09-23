#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "dmn/detail/runtime.hpp"
#include "dmn/detail/uhandle.hpp"
#include "dmn/acl/names.hpp"
#include "dmn/acl/flag.hpp"
#include "dmn/database.hpp"

namespace dmn::acl {
class role_map;
class role;
class entry;
class access;

/// Manage a database access control list.
///
/// \throws dmn::invalid_handle If an underlying handle is empty.
/// \throws dmn::native_error In case of a lower level failure.
class manager : private detail::runtime {
 public:
  using handle_t = dmn::detail::dhandle_t;
  manager() = delete;

  /// Look up the access for a names list.
  [[nodiscard]] auto lookup_access(const dmn::acl::names& names) const -> acl::access;

  /// Return all entries in the ACL.
  [[nodiscard]] auto entries() const -> std::vector<entry>;

  /// Get an entry from the ACL, or an empty result if it does not exist.
  [[nodiscard]] auto get_entry(std::string_view name) const -> std::optional<entry>;

  /// Add an ACL entry to the ACL and return it.
  ///
  /// \throws dmn::invalid_argument If one or more assigned roles do not exist.
  [[nodiscard]] auto add_entry(
    std::string_view name, acl::access value, principal_type type = principal_type::unspecified
  ) const -> entry;

  /// Add a new basic entry without access or roles.
  [[nodiscard]] auto add_entry(std::string_view name, acl::level value) const -> entry;

  /// Replace an entry in-place. If the entry has a new name, it is renamed.
  ///
  /// \throws dmn::invalid_argument If one or more assigned roles do not exist.
  void update_entry(std::string_view name, const entry& value) const;

  /// Return all currently defined roles.
  [[nodiscard]] auto roles() const -> dmn::acl::role_map;

  /// Get the admin server for this ACL.
  [[nodiscard]] auto admin_server() const -> std::string;

  /// Set the admin server for this ACL.
  void set_admin_server(std::string_view server) const;

  /// Persist all in-memory changes to the database.
  void save();

  [[nodiscard]] auto get_handle() const -> handle_t { return hdl_ ? hdl_->get() : handle_t{}; }

 private:
  dmn::database db_;
  using managed_handle_t = dmn::detail::uhandle<handle_t>;
  std::shared_ptr<managed_handle_t> hdl_;
  bool newly_created_;

  static auto read(const dmn::database& db) -> manager;
  static auto create(const dmn::database& db) -> manager;

  manager(dmn::database db, handle_t handle, bool newly_created);

  friend class dmn::database;
};
}  // namespace dmn::acl