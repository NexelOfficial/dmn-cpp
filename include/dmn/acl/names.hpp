#pragma once

#include <cstdint>
#include <vector>
#include <optional>
#include <string>

namespace dmn::acl {
enum class authentication_state : uint8_t {
  /// User is not authenticated
  unauthenticated,
  /// User is authenticated as if they logged in with their password on the web
  password,
  /// User is authenticated as if they were using the Notes client
  notes,
  /// User has full admin access
  admin
};

class names {
 public:
  names();

  /// Build a names list for a username.
  ///
  /// \param name Username to create the list for.
  /// \return Constructed names list.
  /// \throws dmn::error If the names list cannot be created.
  static auto from_username(const std::string& name) -> dmn::acl::names;

  /// Set the authentication state for the list.
  ///
  /// \param state Authentication state to apply. See `dmn::authentication_state` for details.
  void set_authentication(authentication_state state);

  [[nodiscard]] auto get_name(size_t index) const -> std::optional<std::string>;

  // Get the amount of names in the list.
  [[nodiscard]] auto get_count() const -> size_t;

  /// Get the underlying buffer.
  [[nodiscard]] auto buffer() -> std::vector<uint8_t>&;

  /// Get the underlying buffer.
  [[nodiscard]] auto buffer() const -> const std::vector<uint8_t>&;

 private:
  std::vector<uint8_t> buffer_;
};
}  // namespace dmn::acl