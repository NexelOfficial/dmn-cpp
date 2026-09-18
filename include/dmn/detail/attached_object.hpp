#pragma once

#include <optional>

#include "dmn/detail/locker.hpp"
#include "dmn/database.hpp"
#include "dmn/note.hpp"

namespace dmn::detail {
class attached_object {
 public:
  enum class type : uint16_t {
    file = 0,
    filter_left_to_do = 3,
    assist_run_data = 8,
    unknown = 0xFFFF
  };

  using handle_t = uint32_t;
  attached_object() = delete;

  /// Allocate a new attached object in the provided database.
  ///
  /// \param db The database to allocate in.
  /// \param object_type Type of the object.
  /// \param size Size of the object.
  /// \throws dmn::native_error If the allocation failed.
  attached_object(dmn::database db, type object_type, size_t size);

  /// Append the object to a note item.
  ///
  /// \param note The note to append to.
  /// \param key Key to append the object to.
  /// \throws dmn::native_error If the appending failed.
  void append_to_note(const dmn::note& note, std::string_view key) const;

  /// Reallocate the object memory to resize it.
  ///
  /// \param size New size of the object.
  /// \throws dmn::native_error If the reallocation failed.
  /// \note The underlying handle will remain the same.
  void reallocate(size_t size);

  /// Write data to the object.
  ///
  /// \param locker Locker holding data to write.
  /// \throws dmn::native_error If the writing failed.
  /// \throws dmn::invalid_argument If the provided locker does not own the memory.
  void write(detail::locker locker);

  /// Get the size of the object memory.
  ///
  /// \param object_type Type of the object.
  /// \throws dmn::native_error If obtaining the size failed.
  [[nodiscard]] auto size() const -> size_t;

  /// Release the underlying handle.
  [[nodiscard]] auto release() -> handle_t { return hdl_.release(); }

 private:
  type object_type_;
  dmn::database db_;
  detail::uhandle<handle_t> hdl_;

  /// Internal implementation used by `dmn::database`.
  static auto open(const dmn::database& db, std::string_view name) -> std::optional<agent>;
};
}  // namespace dmn::detail