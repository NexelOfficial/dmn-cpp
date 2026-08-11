#pragma once

#include <concepts>
#include <string>
#include <utility>
#include <vector>

#include "dmn/detail/uhandle.hpp"

namespace dmn {
template <typename T>
concept is_mime_note = requires(T note) {
  { note.get_handle() } -> std::convertible_to<detail::dhandle_t>;
};

class mime {
 public:
  using handle_t = void*;
  mime() = delete;

  /// Create and open a MIME stream for a note-like object.
  ///
  /// \param note Note to open the MIME stream for.
  /// \return Instance of `dmn::mime` with an open MIME stream.
  /// \throws dmn::error If opening the MIME stream fails.
  /// \throws std::runtime_error If the underlying handle is empty.
  /// \note Caller is required to keep both the returned `dmn::mime` instance and `note` open until
  /// the `dmn::mime` instance is destroyed.
  template <typename T>
    requires is_mime_note<T>
  static auto open(const T& note) -> mime {
    return open_impl(note.get_handle());
  }

  /// Set the MIME content type.
  ///
  /// \param content_type Content type to write.
  /// \return Instance `dmn::mime` with applied content-type.
  auto set_content_type(std::string content_type) -> mime&;

  /// Set the MIME character set.
  ///
  /// \param charset Character set to write.
  /// \return Instance `dmn::mime` with applied charset.
  auto set_charset(std::string charset) -> mime&;

  /// Append content to the MIME body.
  ///
  /// \param content Content to append.
  /// \return Instance `dmn::mime` with appended content.
  auto append_content(std::string content) -> mime&;

  /// Append queued lines, itemize the MIME stream, and write it to a note field.
  ///
  /// \param note Note to write the field to.
  /// \param field Name of the field to write.
  /// \throws dmn::error If appending to the note fails.
  /// \throws std::runtime_error If writing the content fails.
  /// \throws std::runtime_error If the underlying handle is empty.
  template <typename T>
    requires is_mime_note<T>
  void write_to(const T& note, std::string field) const {
    return write_to_impl(note.get_handle(), std::move(field));
  }

 private:
  detail::uhandle<handle_t> hdl_;
  std::vector<std::string> content_;
  std::string charset_ = "UTF-8";
  std::string content_type_ = "text/html; charset=UTF-8";

  /// Internal implementation used by `dmn::mime::open`.
  [[nodiscard]] static auto open_impl(detail::dhandle_t handle) -> handle_t;

  /// Internal implementation used by `dmn::mime::write_to`.
  void write_to_impl(detail::dhandle_t handle, std::string field) const;

  /// Internal implementation used by `dmn::mime::write_to`.
  void write_line(std::string line) const;

  mime(handle_t handle);
};
}  // namespace dmn
