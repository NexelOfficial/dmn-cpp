#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "dmn/detail/scoped_handle.hpp"
#include "dmn/detail/locker.hpp"
#include "dmn/detail/runtime.hpp"

namespace dmn::acl {
enum class authentication_state : uint8_t {
  /// User is not authenticated
  unauthenticated,
  /// User is authenticated as if they logged in with their password on the web
  password,
  /// User is authenticated as if they were using the Notes client
  notes,
  /// User has full admin access
  admin,
};

/// In-memory list of names and groups.
///
/// \throws dmn::invalid_handle If an underlying handle is empty.
/// \throws dmn::native_error In case of a lower level failure.
class names : private detail::runtime {
 public:
  using handle_t = detail::dhandle_t;
  using value_type = std::string;

  class const_iterator {
   public:
    using value_type = names::value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_concept = std::forward_iterator_tag;
    using iterator_category = std::forward_iterator_tag;

    const_iterator() = default;
    const_iterator(const names* owner, size_t index) : owner_(owner), index_(index) {}

    auto operator*() const -> std::string { return owner_->at(index_); }

    auto operator++() -> const_iterator& {
      ++index_;
      return *this;
    }

    auto operator++(int) -> const_iterator {
      auto copy = *this;
      ++(*this);
      return copy;
    }

    auto operator==(const const_iterator& other) const noexcept -> bool = default;

   private:
    const names* owner_{};
    size_t index_{};
  };

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
  [[nodiscard]] auto at(size_t index) const -> std::string;

  /// Try to get a name in the list at an index.
  [[nodiscard]] auto try_at(size_t index) const -> std::optional<std::string>;

  /// Get the amount of names in the list.
  [[nodiscard]] auto size() const -> size_t;

  [[nodiscard]] auto begin() const -> const_iterator { return {this, 0}; }
  [[nodiscard]] auto end() const -> const_iterator { return {this, size()}; }

  /// Release the underlying handle.
  [[nodiscard]] auto release() -> handle_t { return hdl_.release(); }

  /// Create a copy of the names list.
  [[nodiscard]] auto clone() const -> names {
    auto copy = get_cursor().clone();
    return names{copy.release().pool};
  };

  [[nodiscard]] auto get_cursor() const -> detail::locker {
    return {get_handle(), detail::ownership::borrow};
  }

  [[nodiscard]] auto get_handle() const -> handle_t { return hdl_.get(); }

 private:
  detail::scoped_handle<handle_t> hdl_;

  explicit names(handle_t handle);
};
}  // namespace dmn::acl
