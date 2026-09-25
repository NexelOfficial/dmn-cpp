#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

#include "dmn/detail/locker.hpp"
#include "dmn/detail/runtime.hpp"
#include "dmn/detail/uhandle.hpp"

namespace dmn {
namespace detail {
template <typename T>
struct object_value;
}

/// In-memory Domino text list.
///
/// \throws dmn::invalid_handle If the underlying handle is empty.
/// \throws dmn::native_error In case of a lower level failure.
/// \throws dmn::out_of_range If an operation exceeds the list's bounds or capacity.
class list : protected detail::runtime {
 public:
  class const_iterator {
   public:
    const_iterator() = default;
    const_iterator(const list* owner, size_t index) : owner_(owner), index_(index) {}

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
    friend auto operator==(const const_iterator& lhs, const const_iterator& rhs) -> bool {
      return lhs.owner_ == rhs.owner_ && lhs.index_ == rhs.index_;
    }
    friend auto operator!=(const const_iterator& lhs, const const_iterator& rhs) -> bool {
      return !(lhs == rhs);
    }

   private:
    const list* owner_{};
    size_t index_{};
  };

  /// Create an empty text list in memory.
  list();

  /// Check whether the list contains no entries.
  [[nodiscard]] auto empty() const -> bool;

  /// Get the number of entries in the list.
  ///
  /// \return Number of text entries.
  [[nodiscard]] auto size() const -> size_t;

  /// Get the raw Domino list size.
  ///
  /// \return Size of the list buffer in bytes.
  [[nodiscard]] auto buffer_size() const -> uint16_t;

  /// Get an entry by index.
  [[nodiscard]] auto at(size_t index) const -> std::string;

  /// Append an entry to the list.
  ///
  /// \param value Entry to add.
  void push_back(std::string_view value);

  /// Remove the last entry from the list.
  void pop_back();

  /// Remove the entry at an index.
  void erase(size_t index);

  /// Remove all entries from the list.
  void clear();

  /// Release the list handle.
  void release() { hdl_.release(); }

  [[nodiscard]] auto begin() const -> const_iterator;
  [[nodiscard]] auto end() const -> const_iterator;
  [[nodiscard]] auto cbegin() const -> const_iterator;
  [[nodiscard]] auto cend() const -> const_iterator;

  [[nodiscard]] auto get_cursor() const -> detail::locker {
    return {get_handle(), buffer_size(), detail::ownership::borrow};
  }

  [[nodiscard]] auto get_handle() const -> detail::dhandle_t { return hdl_.get(); }

 private:
  detail::uhandle<detail::dhandle_t> hdl_;
  uint16_t size_ = 0;

  list(std::span<std::byte> buffer);

  friend struct detail::object_value<dmn::list>;
};
}  // namespace dmn
