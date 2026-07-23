#pragma once

#include <vector>
#include <optional>

#include "dmn/nsf/database.hpp"
#include "dmn/nos/object.hpp"

namespace dmn {
class database;

constexpr static uint8_t MAX_TUMBLER_LEVELS = 32;

class view {
 public:
  struct entry {
    dmn::note_id::value_t noteid = 0;
    std::vector<dmn::object> columns;
  };

  using function_t = std::function<void(dmn::note note, const std::vector<dmn::object>& columns)>;
  using handle_t = unsigned short;
  view() = delete;

  /// Iterate over view entries matching a key.
  ///
  /// \param key Key to search for.
  /// \param func Iteration function to call.
  /// \throws dmn::error If the view search fails.
  /// \throws std::runtime_error If the underlying handle is empty.
  void iterate(std::string_view key, const function_t& func) const;

  /// Get all entries in the view.
  ///
  /// \return Entries contained in the view.
  /// \throws std::runtime_error If the underlying handle is empty.
  [[nodiscard]] auto get_entries() const -> std::vector<entry>;

  /// Get entries matching a key.
  ///
  /// \param key Key to search for.
  /// \return Matching view entries. Returns an empty vector if no matches are found.
  /// \throws dmn::error If the view search fails.
  /// \throws std::runtime_error If the underlying handle is empty.
  [[nodiscard]] auto get_entries(std::string_view key) const -> std::vector<entry>;

  [[nodiscard]] auto get_handle() const -> handle_t { return hdl_.get(); }

 private:
  struct key_buffer_data;

  dmn::database db_;
  dmn::uhandle<handle_t> hdl_;

  /// Internal implementation used by `dmn::database`.
  static auto open(const dmn::database& db, std::string_view view_name) -> std::optional<view>;

  /// Internal implementation used by get-functions.
  [[nodiscard]] auto collect_entries(void* position_ptr, uint32_t count, uint32_t skip_count) const
    -> std::vector<entry>;

  view(dmn::database db, handle_t handle);

  friend class database;
};
}  // namespace dmn