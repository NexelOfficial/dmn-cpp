#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "dmn/detail/locker.hpp"
#include "dmn/detail/runtime.hpp"
#include "dmn/detail/uhandle.hpp"

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

/// In-memory list of names and groups.
///
/// \throws dmn::invalid_handle If an underlying handle is empty.
/// \throws dmn::native_error In case of a lower level failure.
class names : private detail::runtime {
 public:
  using handle_t = detail::dhandle_t;

  /// Build a names list for a username.
  ///
  /// \param name Username to create the list for.
  /// \return Constructed names list.
  static auto from_username(std::string_view name) -> dmn::acl::names;

  /// Set the authentication state for the list.
  ///
  /// \param state Authentication state to apply. See `dmn::authentication_state` for details.
  void set_authentication(authentication_state state) const;

  /// Get a name in the list at an index.
  [[nodiscard]] auto get_name(size_t index) const -> std::optional<std::string>;

  /// Get the amount of names in the list.
  [[nodiscard]] auto get_count() const -> size_t;

  /// Release the underlying handle.
  [[nodiscard]] auto release() -> handle_t { return hdl_.release(); }

  /// Get the size of the underlying memory.
  [[nodiscard]] auto size() const -> size_t;

  [[nodiscard]] auto get_cursor() const -> detail::locker {
    return {get_handle(), detail::ownership::borrow};
  }

  [[nodiscard]] auto get_handle() const -> handle_t { return hdl_.get(); }

 private:
  detail::uhandle<handle_t> hdl_;

  names(handle_t handle);
};
}  // namespace dmn::acl