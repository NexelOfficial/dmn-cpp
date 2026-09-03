#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "dmn/detail/uhandle.hpp"
#include "dmn/acl/manager.hpp"
#include "dmn/acl/access.hpp"

namespace dmn::acl {
/// A live entry in a manager's in-memory ACL.
///
/// Setters update the in-memory ACL immediately. Call `manager::save()` to persist those changes
/// to the database.
struct entry {
 public:
  using handle_t = dmn::detail::dhandle_t;
  entry() = delete;
  entry(
    manager mgr, std::string name, acl::access access, principal_type type,
    bool administration_server
  );

  /// Empty for Domino's default ACL entry.
  [[nodiscard]] auto get_name() const noexcept -> std::string_view;
  [[nodiscard]] auto get_access() const noexcept -> const acl::access&;
  [[nodiscard]] auto get_level() const noexcept -> acl::level;
  [[nodiscard]] auto get_flags() const noexcept -> uint16_t;
  [[nodiscard]] auto get_roles() const noexcept -> const std::vector<acl::role>&;
  [[nodiscard]] auto get_type() const noexcept -> principal_type;
  [[nodiscard]] auto is_administration_server() const noexcept -> bool;

  auto set_name(std::string_view name) -> entry&;
  auto set_access(acl::access value) -> entry&;
  auto set_level(acl::level value) -> entry&;
  auto set_flags(uint16_t value) -> entry&;
  auto set_roles(std::vector<acl::role> value) -> entry&;
  auto set_type(principal_type value) -> entry&;
  auto set_administration_server(bool value) -> entry&;
  auto remove() -> bool;

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