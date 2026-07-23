#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "dmn/os/uhandle.hpp"

namespace dmn {
class list {
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
  ///
  /// \throws dmn::error If allocating the list fails.
  list();

  /// Create an in-memory text list from an existing one.
  ///
  /// \param existing Pointer to an existing list prefixed by data type.
  /// \throws dmn::error If allocating the list fails.
  /// \throws std::runtime_error If the underlying handle is empty.
  /// \throws std::invalid_argument If the provided pointer is not of a text list.
  list(void* existing);

  /// Check whether the list contains no entries.
  ///
  /// \return true if the list is empty, otherwise false.
  /// \throws std::runtime_error If the underlying handle is empty.
  [[nodiscard]] auto empty() const -> bool;

  /// Get the number of entries in the list.
  ///
  /// \return Number of text entries.
  /// \throws std::runtime_error If the underlying handle is empty.
  [[nodiscard]] auto size() const -> size_t;

  /// Get the raw Domino list size.
  ///
  /// \return Size of the list buffer in bytes.
  /// \throws std::runtime_error If the underlying handle is empty.
  [[nodiscard]] auto buffer_size() const -> uint16_t;

  /// Get an entry by index.
  ///
  /// \param index Zero-based entry index.
  /// \return Entry at the provided index.
  /// \throws dmn::error If the entry cannot be retrieved.
  /// \throws std::out_of_range If the index is outside the list.
  /// \throws std::runtime_error If the underlying handle is empty.
  [[nodiscard]] auto at(size_t index) const -> std::string;

  /// Append an entry to the list.
  ///
  /// \param value Entry to add. The value is converted from UTF-8 to LMBCS.
  /// \throws dmn::error If the entry cannot be added.
  /// \throws std::out_of_range If the entry is too large for a Domino text list.
  /// \throws std::runtime_error If the underlying handle is empty.
  void push_back(const std::string& value);

  /// Remove the last entry from the list.
  ///
  /// \throws dmn::error If the entry cannot be removed.
  /// \throws std::out_of_range If the list is empty.
  /// \throws std::runtime_error If the underlying handle is empty.
  void pop_back();

  /// Remove the entry at an index.
  ///
  /// \param index Zero-based entry index.
  /// \throws dmn::error If the entry cannot be removed.
  /// \throws std::out_of_range If the index is outside the list.
  /// \throws std::runtime_error If the underlying handle is empty.
  void erase(size_t index);

  /// Remove all entries from the list.
  ///
  /// \throws dmn::error If the list cannot be cleared.
  /// \throws std::runtime_error If the underlying handle is empty.
  void clear();

  /// Release the list handle.
  ///
  /// Sets the handle to a null value preventing the cleanup function from being called.
  void release() noexcept { hdl_.release(); }

  [[nodiscard]] auto begin() const -> const_iterator;
  [[nodiscard]] auto end() const -> const_iterator;
  [[nodiscard]] auto cbegin() const -> const_iterator;
  [[nodiscard]] auto cend() const -> const_iterator;

  [[nodiscard]] auto get_handle() const -> dmn::dhandle_t { return hdl_.get(); }

 private:
  dmn::uhandle<dmn::dhandle_t> hdl_;
  uint16_t size_ = 0;
};
}  // namespace dmn
