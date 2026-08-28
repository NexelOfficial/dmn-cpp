#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <regex>
#include <string>
#include <unordered_map>

#include "dmn/detail/note_value.hpp"
#include "dmn/detail/object_value.hpp"
#include "dmn/detail/uhandle.hpp"
#include "dmn/lmbcs.hpp"
#include "dmn/object.hpp"
#include "dmn/type.hpp"
#include "dmn/database.hpp"
#include "dmn/unid.hpp"

namespace dmn {
class strlist;

class note {
 public:
  using object_map_t = std::unordered_map<std::string, dmn::object>;
  using handle_t = detail::dhandle_t;

  note() = delete;

  /// Check whether an item exists on the note.
  ///
  /// \param key Item to look up.
  /// \return true if the item exists; otherwise false.
  /// \throws dmn::invalid_handle If the underlying handle is empty.
  [[nodiscard]] auto has(std::string_view key) const -> bool;

  /// Copy this note to another database.
  ///
  /// \param db Target database.
  /// \return Copy of the note in the target database.
  /// \throws dmn::native_error If the note cannot be copied.
  /// \throws dmn::runtime_error If the new note doesn't have a note id.
  /// \throws dmn::invalid_handle If an underlying handle is empty.
  [[nodiscard]] auto copy_to_database(const dmn::database& db) const -> std::optional<note>;

  /// Get the type of an item.
  ///
  /// \param key Item name to get the type for
  /// \throws dmn::invalid_handle If the underlying handle is empty.
  [[nodiscard]] auto get_type(std::string_view key) const -> dmn::type;

  /// Erase an item from the note.
  ///
  /// \param key Item name to erase.
  /// \throws dmn::native_error If the item cannot be erased.
  /// \throws dmn::invalid_handle If the underlying handle is empty.
  void erase(std::string_view key) const;

  /// Embed a file attachment in the note.
  ///
  /// \param name Attachment name stored in the note.
  /// \param path Path to the file to attach.
  /// \throws dmn::native_error If the attachment cannot be embedded.
  /// \throws dmn::invalid_handle If the underlying handle is empty.
  void embed_element(std::string_view name, const std::filesystem::path& path) const;

  /// Embed a file attachment in the note with a random name.
  ///
  /// \param path Path to the file to attach.
  /// \throws dmn::native_error If the attachment cannot be embedded.
  /// \throws dmn::invalid_handle If the underlying handle is empty.
  /// \note File extension of the provided path is preserved.
  void embed_element(const std::filesystem::path& path) const;

  /// Compute the note using its associated form.
  ///
  /// \throws dmn::native_error If form computation fails.
  /// \throws dmn::invalid_handle If the underlying handle is empty.
  void compute_with_form() const;

  /// Save the note to the database.
  ///
  /// \param force Whether to force the update.
  /// \throws dmn::native_error If the note cannot be saved.
  /// \throws dmn::invalid_handle If the underlying handle is empty.
  void save(bool force) const;

  /// Delete the note from the database.
  ///
  /// \param force Whether to force the deletion.
  /// \throws dmn::native_error If the note cannot be deleted.
  /// \throws dmn::invalid_handle If an underlying handle is empty.
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
  /// \throws dmn::native_error If the existing item cannot be removed or the new value cannot be
  /// stored.
  /// \throws dmn::invalid_handle If the underlying handle is empty.
  template <typename T>
    requires detail::has_note_value_apply<T>
  void set(std::string_view key, const T& value) const {
    detail::note_value<T>::apply(value, [&](auto type, auto buffer) {
      auto func = has(key) ? &dmn::note::modify_impl : &dmn::note::append_impl;
      std::invoke(func, this, key, type, buffer);
    });
  }

  /// Get an item's value as a certain type.
  ///
  /// \param key Item name to retrieve.
  /// \return Retrieved item, if available.
  /// \throws dmn::invalid_handle If the underlying handle is empty.
  /// \note When getting the item as a string, all non-string types are converted to string
  /// automatically thus the type is not checked.
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
  /// \throws dmn::invalid_handle If the underlying handle is empty.
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

  [[nodiscard]] auto get_database() const -> const dmn::database& { return db_; }

  [[nodiscard]] auto try_get_handle() const noexcept -> std::optional<handle_t> {
    return hdl_->try_get();
  }

  [[nodiscard]] auto get_handle() const -> handle_t { return hdl_ ? hdl_->get() : handle_t{}; }

 private:
  dmn::database db_;

  using managed_handle_t = detail::uhandle<handle_t>;
  std::shared_ptr<managed_handle_t> hdl_;

  note(dmn::database db, handle_t handle);

  /// Internal implementation used by `dmn::database`.
  static auto open(dmn::database db, dmn::unid unid) -> std::optional<note>;
  /// Internal implementation used by `dmn::database`.
  static auto open(dmn::database db, dmn::note_id noteid) -> std::optional<note>;
  /// Internal implementation used by `dmn::database`.
  static auto create(dmn::database db) -> note;

  /// Internal implementation used by `dmn::note::get_info()`.
  void get_info_impl(dmn::info key, void* out) const;

  /// Internal implementation used by `dmn::note::get()`.
  [[nodiscard]] auto get_impl(dmn::lmbcs_view key) const -> std::optional<dmn::object>;

  /// Internal implementation used by `dmn::note::set()`.
  void append_impl(std::string_view key, dmn::type type, std::span<const std::byte> buffer) const;
  /// Internal implementation used by `dmn::note::set()`.
  void modify_impl(std::string_view key, dmn::type type, std::span<const std::byte> buffer) const;

  template <typename>
  static constexpr bool always_false = false;

  template <dmn::info>
  static constexpr bool always_false_info = false;

  friend class database;
  friend class view;
};
}  // namespace dmn