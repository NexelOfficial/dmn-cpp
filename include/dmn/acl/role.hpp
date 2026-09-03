#pragma once

#include <iterator>
#include <string>

#include "dmn/acl/manager.hpp"
#include "dmn/error.hpp"

namespace dmn::acl {
class role {
 public:
  constexpr explicit role(std::string name) : name_(normalize(std::move(name))) {
    if (name_.empty()) {
      throw dmn::invalid_argument("ACL role name cannot be empty");
    }
  }

  [[nodiscard]] auto name() const noexcept -> std::string_view { return name_; }

  friend auto operator==(const role&, const role&) -> bool = default;

 private:
  std::string name_;

  [[nodiscard]] constexpr static auto normalize(std::string name) -> std::string {
    if (name.size() >= 2 && name.front() == '[' && name.back() == ']') {
      return name.substr(1, name.size() - 2);
    }
    return name;
  }
};

/// Container type that manages ACL roles under the hood
///
/// \throws dmn::invalid_handle If the underlying handle is empty.
class role_map {
 public:
  using value_type = std::pair<size_t, acl::role>;

  class const_iterator {
   public:
    using value_type = role_map::value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_concept = std::input_iterator_tag;
    using iterator_category = std::input_iterator_tag;

    const_iterator() = default;

    const_iterator(const role_map* owner, size_t index) : owner_(owner), index_(index) {
      skip_empty();
    }

    [[nodiscard]] auto operator*() const -> value_type { return {index_, owner_->at(index_)}; }

    auto operator++() -> const_iterator& {
      ++index_;
      skip_empty();
      return *this;
    }

    auto operator++(int) -> const_iterator {
      auto copy = *this;
      ++(*this);
      return copy;
    }

    friend auto operator==(const const_iterator& lhs, const const_iterator& rhs) -> bool {
      return lhs.owner_ == rhs.owner_ && lhs.index_ == rhs.index_;
    }

    friend auto operator!=(const const_iterator& lhs, const const_iterator& rhs) -> bool {
      return !(lhs == rhs);
    }

   private:
    void skip_empty() {
      if (owner_ == nullptr) {
        return;
      }

      while (index_ < owner_->capacity() && !owner_->try_at(index_)) {
        ++index_;
      }
    }

    const role_map* owner_{};
    size_t index_{};
  };

  /// Maximum number of role slots.
  [[nodiscard]] auto capacity() const noexcept -> size_t;

  /// Number of populated role slots.
  [[nodiscard]] auto size() const -> size_t;

  /// Whether the list is empty or not.
  [[nodiscard]] auto empty() const -> bool { return size() == 0; }

  /// Get a role by slot.
  [[nodiscard]] auto at(size_t key) const -> acl::role;

  /// Try to get a role by slot.
  [[nodiscard]] auto try_at(size_t key) const noexcept -> std::optional<acl::role>;

  /// Insert a role at the first available position.
  void insert(const role& value);

  /// Set/replace the role at a slot.
  void set(size_t key, const acl::role& value);

  /// Remove the role at a slot.
  void erase(size_t key);

  /// Remove every role.
  void clear();

  [[nodiscard]] auto begin() const -> const_iterator { return const_iterator{this, 0}; }
  [[nodiscard]] auto end() const -> const_iterator { return const_iterator{this, capacity()}; }

 private:
  acl::manager mgr_;

  role_map(acl::manager mgr) : mgr_(std::move(mgr)) {};

  friend class dmn::acl::manager;
};
}  // namespace dmn::acl