#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <utility>
#include <regex>
#include <string>
#include <unordered_map>

#include "dmn/detail/note_value.hpp"
#include "dmn/detail/object_value.hpp"
#include "dmn/detail/runtime.hpp"
#include "dmn/detail/uhandle.hpp"
#include "dmn/flags.hpp"
#include "dmn/lmbcs.hpp"
#include "dmn/object.hpp"
#include "dmn/type.hpp"
#include "dmn/database.hpp"
#include "dmn/unid.hpp"

namespace dmn {
class strlist;

/// Domino note and its items.
///
/// \throws dmn::invalid_handle If an underlying handle is empty.
/// \throws dmn::native_error In case of a lower level failure.
class note : protected detail::runtime {
 public:
  using object_map_t = std::unordered_map<std::string, dmn::object>;
  using handle_t = detail::dhandle_t;
  struct state {
    dmn::database db;
    dmn::note_id note_id;
    std::optional<detail::uhandle<handle_t>> hdl;
  };

  note() = delete;

  /// Check whether an item exists on the note.
  ///
  /// \param key Item to look up.
  /// \return true if the item exists; otherwise false.
  [[nodiscard]] auto has(std::string_view key) const -> bool;

  /// Copy this note to another database.
  ///
  /// \param other Target database.
  /// \return Copy of the note in the target database.
  [[nodiscard]] auto copy_to_database(dmn::database other) const -> std::optional<note>;

  /// Get the type of an item.
  ///
  /// \param key Item name to get the type for
  [[nodiscard]] auto get_type(std::string_view key) const -> dmn::type;

  /// Erase an item from the note.
  ///
  /// \param key Item name to erase.
  void erase(std::string_view key) const;

  /// Embed a file attachment in the note.
  ///
  /// \param name Attachment name stored in the note.
  /// \param path Path to the file to attach.
  void embed_element(std::string_view name, const std::filesystem::path& path) const;

  /// Embed a file attachment in the note with a random name.
  ///
  /// \param path Path to the file to attach.
  /// \note File extension of the provided path is preserved.
  void embed_element(const std::filesystem::path& path) const;

  /// Compute the note using its associated form.
  void compute_with_form() const;

  /// Sign the note.
  void sign() const;

  /// Save the note to the database.
  ///
  /// \param force Whether to force the update.
  void save(bool force) const;

  /// Delete the note from the database.
  ///
  /// \param force Whether to force the deletion.
  void remove(bool force) const;

  /// Collect all items of the note to a map.
  ///
  /// \param pattern Optional regex pattern to match keys to.
  /// \return Scan results that contain the item name and type.
  [[nodiscard]] auto items(std::optional<std::regex> pattern) const -> object_map_t;

  /// Set an item of the note.
  ///
  /// \param key Item name.
  /// \param value Value to set.
  /// \throws dmn::invalid_argument If an existing item disappears before it can be modified.
  template <typename T, typename... Args>
    requires detail::has_note_value_apply<T> && (std::is_same_v<Args, dmn::item_flag> && ...)
  void set(std::string_view key, const T& value, Args... flags) const {
    auto joined_flags = (uint16_t{} | ... | std::to_underlying(flags));
    detail::note_value<T>::apply(value, [&](auto type, auto buffer) {
      auto func = has(key) ? &dmn::note::modify_impl : &dmn::note::append_impl;
      std::invoke(func, this, key, type, buffer, joined_flags);
    });
  }

  /// Get an item's value as a certain type.
  ///
  /// \param key Item name to retrieve.
  /// \return Retrieved item, if available.
  template <typename T>
    requires detail::has_object_convert<T> || std::is_same_v<T, dmn::object>
  [[nodiscard]] auto get(std::string_view key) const -> std::optional<T> {
    const auto converted = dmn::lmbcs::from_string(key);
    auto value = get_impl(converted);
    if (!value) {
      return std::nullopt;
    }

    if constexpr (detail::has_object_convert<T>) {
      return value->try_as<T>();
    } else if constexpr (std::is_same_v<T, dmn::object>) {
      return value;
    }
    return std::nullopt;
  }

  /// Get information about the note.
  ///
  /// \return Retrieved information
  template <dmn::info Info>
  [[nodiscard]] auto info() const {
    if constexpr (Info == dmn::info::note_id) {
      dmn::note_id value{};
      get_info_impl(dmn::info::note_id, &value);
      return value;
    } else if constexpr (Info == dmn::info::oid) {
      dmn::oid value{};
      get_info_impl(dmn::info::oid, &value);
      return value;
    } else if constexpr (Info == dmn::info::unid) {
      dmn::oid value{};
      get_info_impl(dmn::info::oid, &value);
      return value.universalid;
    } else {
      static_assert(note::always_false_info<Info>, "Unsupported type for get_info");
    }
  }

  [[nodiscard]] auto get_database() const -> const dmn::database& { return state_->db; }

  [[nodiscard]] auto get_handle() const -> handle_t;

 private:
  std::shared_ptr<state> state_;
  note(state st) : state_(std::make_shared<state>(std::move(st))) {};

  [[nodiscard]] auto open_impl() const -> std::optional<handle_t>;
  static auto open(dmn::database db, dmn::unid unid) -> std::optional<note>;
  static auto open(dmn::database db, dmn::note_id note_id) -> std::optional<note>;
  static auto create(dmn::database db) -> note;

  void get_info_impl(dmn::info key, void* out) const;

  [[nodiscard]] auto get_impl(dmn::lmbcs_view key) const -> std::optional<dmn::object>;

  void append_impl(
    std::string_view key, dmn::type type, std::span<const std::byte> buffer, uint16_t flags
  ) const;

  void modify_impl(
    std::string_view key, dmn::type type, std::span<const std::byte> buffer, uint16_t flags
  ) const;

  template <typename>
  static constexpr bool always_false = false;

  template <dmn::info>
  static constexpr bool always_false_info = false;

  friend class database;
  friend class view;
};
}  // namespace dmn