
#pragma once

#include <concepts>
#include <span>
#include <string>
#include <string_view>
#include <array>

#include "dmn/detail/scoped_handle.hpp"
#include "dmn/detail/runtime.hpp"
#include "dmn/messaging/header.hpp"
#include "dmn/lmbcs.hpp"

namespace dmn {
/// MIME stream for a note and item.
///
/// \throws dmn::native_error In case of a lower level failure.
/// \throws dmn::invalid_handle If the underlying handle is empty.
/// \note Caller is required to keep both the returned `dmn::mime` instance and `note` open until
/// the `dmn::mime` instance is destroyed.
class mimestream : protected detail::runtime {
 public:
  using handle_t = void*;
  mimestream() = delete;

 protected:
  template <typename T>
    requires requires(const T& value) {
      { value.get_handle() } -> std::same_as<detail::dhandle_t>;
    }
  mimestream(const T& note, std::string_view item, bool is_write)
      : mimestream(note.get_handle(), dmn::lmbcs::from_string(item), is_write) {}

  [[nodiscard]] auto get_handle() -> handle_t { return hdl_.get(); }
  [[nodiscard]] auto get_item() -> dmn::lmbcs& { return item_; }
  void finalize_impl(detail::dhandle_t note_handle);

 private:
  detail::scoped_handle<handle_t> hdl_;
  dmn::lmbcs item_;

  mimestream(detail::dhandle_t note_handle, dmn::lmbcs item, bool is_write);
  [[nodiscard]] static auto open_impl(
    detail::dhandle_t note_handle, const dmn::lmbcs& item, bool is_write
  ) -> detail::scoped_handle<handle_t>;
};

/// MIME input stream for a note and item.
///
/// \throws dmn::native_error In case of a lower level failure.
/// \throws dmn::invalid_handle If the underlying handle is empty.

class imimestream : public mimestream {
 public:
  template <typename T>
    requires requires(const T& value) {
      { value.get_handle() } -> std::same_as<detail::dhandle_t>;
    }
  imimestream(const T& note, std::string_view item) : mimestream(note, item, false) {}

  /// Read a whitespace-delimited word from the MIME stream.
  ///
  /// \throws dmn::mime_error If reading the data failed.
  auto operator>>(std::string& out) -> imimestream&;

  /// Read a line from the MIME stream.
  ///
  /// \throws dmn::mime_error If reading the data failed.
  auto getline(std::string& out) -> imimestream&;

  /// Read up to size bytes from the MIME stream.
  ///
  /// \returns Number of bytes read.
  /// \throws dmn::mime_error If reading the data failed.
  auto read(std::span<char> buffer) -> size_t;

  [[nodiscard]] explicit operator bool() const { return !failed_; }
  [[nodiscard]] auto eof() const -> bool { return eof_; }

 private:
  constexpr static uint16_t BUFFER_SIZE = 4096;
  std::array<uint8_t, BUFFER_SIZE> buffer_{};
  size_t position_ = 0;
  size_t size_ = 0;
  bool eof_ = false;
  bool failed_ = false;

  auto get() -> int;
  auto next_chunk() -> bool;
};

/// MIME output stream for a note and item.
///
/// \throws dmn::native_error In case of a lower level failure.
/// \throws dmn::invalid_handle If the underlying handle is empty.
class omimestream : public mimestream {
 public:
  template <typename T>
    requires requires(const T& value) {
      { value.get_handle() } -> std::same_as<detail::dhandle_t>;
    }
  omimestream(const T& note, std::string_view item) : mimestream(note, item, true) {}

  /// Append a header to the MIME stream.
  ///
  /// \throws dmn::mime_error If writing the header fails.
  /// \throws dmn::runtime_error If a header is written after content.
  auto operator<<(dmn::header hdr) -> omimestream&;

  /// Append text data to the MIME stream.
  ///
  /// \throws dmn::mime_error If writing the line failed.
  auto operator<<(std::string_view buffer) -> omimestream&;

  /// Finalize the MIME stream into a note item.
  ///
  /// \param note Note-like object to write the MIME item to.
  /// \throws dmn::mime_error If finalizing the content failed.
  template <typename T>
    requires requires(const T& value) {
      { value.get_handle() } -> std::same_as<detail::dhandle_t>;
    }
  void finalize(const T& note) {
    finalize_impl(note.get_handle());
  }

 private:
  bool has_content_ = false;

  auto write(std::string_view buffer) -> omimestream&;
};
}  // namespace dmn
