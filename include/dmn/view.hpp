#pragma once

#include <functional>
#include <limits>
#include <optional>
#include <string_view>
#include <vector>

#include "dmn/database.hpp"
#include "dmn/object.hpp"

namespace dmn {
class database;

constexpr static uint8_t MAX_TUMBLER_LEVELS = 32;

class view {
 public:
  struct entry {
    dmn::note_id noteid;
    std::vector<dmn::object> columns;
  };

  struct query_options {
    /// Optional key to search for.
    std::optional<std::string_view> key;
    /// Maximum number of entries to return or iterate over.
    uint32_t count = std::numeric_limits<uint32_t>::max();
    /// Number of entries to skip before starting.
    uint32_t offset = 0;
  };

  using function_t = std::function<void(dmn::note, const std::vector<dmn::object>&)>;
  using handle_t = unsigned short;
  view() = delete;

  /// Iterate over view entries.
  ///
  /// \param opts Query options. See `dmn::view::query_options` for details.
  /// \param func Iteration function to call.
  /// \throws dmn::native_error If the view search fails.
  /// \throws dmn::invalid_handle If the underlying handle is empty.
  void iterate(const query_options& opts, const function_t& func) const;

  /// Iterate over view entries.
  ///
  /// \param func Iteration function to call.
  /// \throws dmn::native_error If the view search fails.
  /// \throws dmn::invalid_handle If the underlying handle is empty.
  void iterate(const function_t& func) const { iterate(query_options{}, func); }

  /// Get entries from the view.
  ///
  /// \param opts Query options. See `dmn::view::query_options` for details.
  /// \return Entries found in the view.
  /// \throws dmn::invalid_handle If the underlying handle is empty.
  [[nodiscard]] auto get_entries(const query_options& opts) const -> std::vector<entry>;

  /// Get entries from the view.
  ///
  /// \param opts Query options. See `dmn::view::query_options` for details.
  /// \return Entries found in the view.
  /// \throws dmn::invalid_handle If the underlying handle is empty.
  [[nodiscard]] auto get_entries() const -> std::vector<entry> {
    return get_entries(query_options{});
  }

  [[nodiscard]] auto get_handle() const -> handle_t { return hdl_.get(); }

 private:
  dmn::database db_;
  detail::uhandle<handle_t> hdl_;

  /// Internal implementation used by `dmn::database`.
  static auto open(const dmn::database& db, std::string_view view_name) -> std::optional<view>;

  /// Internal implementation used by `iterate()`.
  void iterate_entries(void* position_ptr, const query_options& opts, const function_t& func) const;

  view(dmn::database db, handle_t handle);

  friend class database;
};
}  // namespace dmn
