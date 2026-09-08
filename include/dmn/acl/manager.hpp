#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "dmn/detail/uhandle.hpp"
#include "dmn/acl/names.hpp"
#include "dmn/acl/flag.hpp"
#include "dmn/database.hpp"

namespace dmn::acl {
class role_map;
class role;
class entry;
class access;

class manager {
 public:
  using handle_t = dmn::detail::dhandle_t;
  manager() = delete;

  /// Look up Domino's effective ACL result for a names list.
  /// Irrelevant native modifier bits are masked for the returned access level.
  [[nodiscard]] auto lookup_access(const dmn::acl::names& names) const -> acl::access;

  /// Return all entries in the in-memory ACL.
  [[nodiscard]] auto entries() const -> std::vector<entry>;

  /// Get an entry from the in-memory ACL, or an empty result if it does not exist.
  [[nodiscard]] auto get_entry(std::string_view name) const -> std::optional<entry>;

  /// Add an ACL entry to the in-memory ACL and return it.
  [[nodiscard]] auto add_entry(
    std::string_view name, acl::access value, principal_type type = principal_type::unspecified
  ) const -> entry;

  /// Compatibility convenience for adding an entry without flags or roles.
  [[nodiscard]] auto add_entry(std::string_view name, acl::level value) const -> entry;

  /// Replace an entry in-place. If the entry has a new name, it is renamed.
  void update_entry(std::string_view name, const entry& value) const;

  /// Return all currently defined modern roles
  [[nodiscard]] auto roles() const -> dmn::acl::role_map;

  [[nodiscard]] auto admin_server() const -> std::string;
  void set_admin_server(std::string_view server) const;

  /// Persist all in-memory mutations to the database.
  void save();

  [[nodiscard]] auto get_handle() const -> handle_t { return hdl_ ? hdl_->get() : handle_t{}; }

 private:
  enum class origin : uint8_t { existing, newly_created };

  dmn::database db_;
  using managed_handle_t = dmn::detail::uhandle<handle_t>;
  std::shared_ptr<managed_handle_t> hdl_;
  origin origin_;

  static auto read(const dmn::database& db) -> manager;
  static auto create(const dmn::database& db) -> manager;

  manager(dmn::database db, handle_t handle, origin source);

  friend class dmn::database;
};
}  // namespace dmn::acl