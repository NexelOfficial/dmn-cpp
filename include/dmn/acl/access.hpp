#pragma once

#include <algorithm>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <vector>

#include "dmn/acl/flag.hpp"
#include "dmn/acl/role.hpp"

namespace dmn::acl {
template <typename... Args>
  requires(std::is_same_v<Args, flag> && ...)
[[nodiscard]] constexpr auto flags(Args... values) noexcept -> uint16_t {
  return (uint16_t{} | ... | static_cast<uint16_t>(values));
}

struct access {
  access() = default;

  template <typename... Args>
    requires((std::is_same_v<Args, acl::flag> || std::is_same_v<Args, acl::role>) && ...)
  explicit access(acl::level value, Args&&... args) : level(value) {
    auto add = [&]<typename T>(T value) {
      if constexpr (std::is_same_v<T, acl::flag>) {
        add_flag(value);
      } else {
        add_role(value);
      }
    };

    (add(std::forward<Args>(args)), ...);
  }

  constexpr void add_flag(acl::flag value) noexcept { flags |= static_cast<uint16_t>(value); }

  constexpr void add_role(acl::role value) {
    if (!has_role(value)) {
      roles.push_back(std::move(value));
    }
  }

  [[nodiscard]] constexpr auto has_flag(acl::flag value) const noexcept -> bool {
    return (static_cast<uint16_t>(flags) & static_cast<uint16_t>(value)) != 0;
  }

  [[nodiscard]] constexpr auto has_role(const acl::role& wanted) const -> bool {
    return std::ranges::find(roles, wanted) != roles.end();
  }

  friend auto operator==(const access&, const access&) -> bool = default;

  acl::level level{acl::level::noaccess};
  std::vector<acl::role> roles;
  uint16_t flags{};
};
}  // namespace dmn::acl