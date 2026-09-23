#pragma once

#include <string>
#include <string_view>

#include "dmn/detail/runtime.hpp"
#include "dmn/acl/manager.hpp"
#include "dmn/acl/access.hpp"

namespace dmn::acl {
/// Entry in a database access control list.
///
/// \throws dmn::invalid_handle If the underlying handle is empty.
/// \throws dmn::native_error In case of a lower level failure.
class entry : private detail::runtime {
 public:
  entry() = delete;
  entry(
    manager mgr, std::string name, acl::access access, principal_type type,
    bool administration_server
  );

  /// Get the name for this entry.
  [[nodiscard]] auto get_name() const noexcept -> std::string_view;

  /// Get the access for this entry.
  [[nodiscard]] auto get_access() const noexcept -> const acl::access&;

  /// Get the base access level for this entry.
  [[nodiscard]] auto get_level() const noexcept -> acl::level;

  /// Get the principal type for this entry.
  [[nodiscard]] auto get_type() const noexcept -> principal_type;

  /// Get whether this entry is an administration server or not.
  [[nodiscard]] auto is_administration_server() const noexcept -> bool;

  /// Rename the current entry.
  auto set_name(std::string_view name) -> entry&;

  /// Update the access for this entry.
  auto set_access(acl::access value) -> entry&;

  /// Set the base access level for this entry.
  auto set_level(acl::level value) -> entry&;

  /// Change the principal type of this entry.
  auto set_type(principal_type value) -> entry&;

  /// Change whether this entry is an administration server or not.
  auto set_administration_server(bool value) -> entry&;

  /// Remove the current entry from the ACL.
  void remove();

  friend auto operator==(const entry& lhs, const entry& rhs) noexcept -> bool {
    return lhs.name_ == rhs.name_ && lhs.access_ == rhs.access_ && lhs.type_ == rhs.type_ &&
           lhs.administration_server_ == rhs.administration_server_;
  }

 private:
  manager mgr_;
  std::string name_;
  acl::access access_;
  principal_type type_;
  bool administration_server_;

  friend class manager;
};
}  // namespace dmn::acl