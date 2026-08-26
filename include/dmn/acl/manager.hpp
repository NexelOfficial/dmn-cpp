#pragma once

#include "dmn/database.hpp"
#include "dmn/detail/uhandle.hpp"
#include "dmn/acl/names.hpp"

namespace dmn::acl {
class manager {
 public:
  manager() = delete;

  /// Look up the effective access level for the specified names list.
  ///
  /// \param names Names list to evaluate.
  /// \return Effective ACL access level.
  /// \throws dmn::native_error If the access lookup fails or the ACL is corrupt.
  [[nodiscard]] auto lookup_access(dmn::acl::names& names) const -> uint16_t;

  [[nodiscard]] auto get_handle() const -> detail::dhandle_t { return hdl_.get(); }

 private:
  dmn::database db_;
  detail::uhandle<detail::dhandle_t> hdl_;

  /// Internal implementation used by `dmn::database`.
  static auto read(const dmn::database& db) -> manager;

  manager(dmn::database db, detail::dhandle_t handle);

  friend class dmn::database;
};
}  // namespace dmn::acl