#pragma once

#include <memory>
#include <optional>
#include <regex>
#include <typeindex>
#include <string>
#include <unordered_map>

#include "dmn/nos/object.hpp"
#include "dmn/nos/type.hpp"
#include "dmn/os/lmbcs.hpp"
#include "dmn/os/uhandle.hpp"
#include "dmn/os/locker.hpp"
#include "dmn/nos/list.hpp"
#include "dmn/nsf/database.hpp"
#include "dmn/nsf/note_lock.hpp"
#include "dmn/misc/unid.hpp"

namespace dmn {
class strlist;

class note {
 public:
  using object_map_t = std::unordered_map<std::string, dmn::object>;
  using handle_t = dmn::dhandle_t;

  note() = delete;

  /// Check whether an item exists on the note.
  ///
  /// \param key Item to look up.
  /// \return true if the item exists; otherwise false.
  /// \throws std::runtime_error If the underlying handle is empty.
  [[nodiscard]] auto has_item(std::string_view key) const -> bool;

  /// Copy this note to another database.
  ///
  /// \param db Target database.
  /// \return Copy of the note in the target database.
  /// \throws dmn::error If the note cannot be copied.
  /// \throws std::runtime_error If an underlying handle is empty.
  [[nodiscard]] auto copy_to_database(const dmn::database& db) const -> std::optional<note>;

  /// Remove an item from the note.
  ///
  /// \param key Item name to remove.
  /// \throws dmn::error If the item cannot be removed.
  /// \throws std::runtime_error If the underlying handle is empty.
  void remove_item(std::string_view key) const;

  /// Embed a file attachment in the note.
  ///
  /// \param name Attachment name stored in the note.
  /// \param path Path to the file to attach.
  /// \throws dmn::error If the attachment cannot be embedded.
  /// \throws std::runtime_error If the underlying handle is empty.
  void embed_element(const std::string& name, const std::string& path) const;

  /// Embed a file attachment in the note with a random name.
  ///
  /// \param path Path to the file to attach.
  /// \throws dmn::error If the attachment cannot be embedded.
  /// \throws std::runtime_error If the underlying handle is empty.
  /// \note File extension of the provided path is preserved.
  void embed_element(const std::string& path) const;

  /// Compute the note using its associated form.
  ///
  /// \throws dmn::error If form computation fails.
  /// \throws std::runtime_error If the underlying handle is empty.
  void compute_with_form() const;

  /// Save the note to the database.
  ///
  /// \param force Whether to force the update.
  /// \throws dmn::error If the note cannot be saved.
  /// \throws std::runtime_error If the underlying handle is empty.
  void save(bool force) const;

  /// Delete the note from the database.
  ///
  /// \param force Whether to force the deletion.
  /// \throws dmn::error If the note cannot be deleted.
  /// \throws std::runtime_error If an underlying handle is empty.
  void remove(bool force) const;

  /// Acquire a lock for the note.
  ///
  /// \throws dmn::error If the note is locked by another instance.
  /// \note Does nothing if the note is already locked by this instance.
  /// \note Uses Domino's Note locking mechanism, which must be enabled for the database.
  void lock();

  /// Attempt to acquire a lock for the note.
  ///
  /// \return true if the note is locked after the call, false otherwise.
  /// \note Uses Domino's Note locking mechanism, which must be enabled for the database.
  auto try_lock() noexcept -> bool;

  /// Release the note lock.
  ///
  /// \throws dmn::error If the note cannot be unlocked.
  /// \note Does nothing if the note is not locked.
  /// \note Uses Domino's Note locking mechanism, which must be enabled for the database.
  void unlock();

  /// Attempt to release the note lock.
  ///
  /// \return true if the note is unlocked after the call, false otherwise.
  /// \note Uses Domino's Note locking mechanism, which must be enabled for the database.
  auto try_unlock() noexcept -> bool;

  /// Collect all items of the note to a map.
  ///
  /// \param pattern Optional regex pattern to match keys to.
  /// \return Scan results that contain the item name and type.
  [[nodiscard]] auto items(std::optional<std::regex> pattern) const -> object_map_t;

  /// Replace an item of the note.
  ///
  /// \param key Item name.
  /// \param value Value to set.
  /// \throws dmn::error If the existing item cannot be removed or the new value cannot be stored.
  /// \throws std::runtime_error If the underlying handle is empty.
  template <typename T>
  void replace_item_value(std::string_view key, const T& value) const {
    if (has_item(key)) {
      remove_item(key);
    }
    append_item_value(key, value);
  }

  /// Append an item to the note.
  ///
  /// \param key Item name.
  /// \param value Value to append.
  /// \throws dmn::error If the value cannot be appended.
  /// \throws std::runtime_error If the underlying handle is empty.
  template <typename T>
  void append_item_value(std::string_view key, const T& value) const {
    const lmbcs::str conv_key = lmbcs::translate(key);
    if constexpr (std::is_convertible_v<T, std::string_view>) {
      auto conv_value = lmbcs::translate(value);
      append_item_value(conv_key, dmn::type::text, conv_value.data(), conv_value.size());
    } else if constexpr (std::is_convertible_v<T, double>) {
      auto converted = static_cast<double>(value);
      append_item_value(conv_key, dmn::type::number, &converted, sizeof(converted));
    } else if constexpr (std::is_same_v<T, dmn::list>) {
      auto list_obj =
        dmn::os::locker(value.get_handle(), value.buffer_size(), dmn::os::ownership::borrow);

      append_item_value(
        conv_key, dmn::type::text_list, list_obj.get_pointer(sizeof(uint16_t)),
        value.buffer_size() - sizeof(uint16_t)
      );
    } else {
      static_assert(std::false_type::value, "Unsupported type for append_item_value");
    }
  }

  /// Get an item's value as a certain type.
  ///
  /// \param key Item name to retrieve.
  /// \return Retrieved item, if available.
  /// \throws std::runtime_error If the underlying handle is empty.
  /// \note When getting the item as a string, all non-string types are converted to string
  /// automatically thus the type is not checked.
  template <typename T>
  [[nodiscard]] auto get_item_value(std::string_view key) const -> std::optional<T> {
    const lmbcs::str converted = lmbcs::translate(key);
    auto value = get_item_value_impl(converted);
    if (!value) {
      return std::nullopt;
    }

    if constexpr (std::is_same_v<T, std::string>) {
      return value->try_as<std::string>();
    } else if constexpr (std::is_convertible_v<T, double>) {
      return value->try_as<T>();
    } else if constexpr (std::is_same_v<T, dmn::list>) {
      return value->try_as<dmn::list>();
    } else if constexpr (std::is_same_v<T, dmn::object>) {
      return value;
    } else {
      static_assert(std::false_type::value, "Unsupported type for get_item_value");
    }
  }

  [[nodiscard]] auto get_database() const -> const dmn::database& { return db_; }

  [[nodiscard]] auto get_universalid() const -> std::string;

  [[nodiscard]] auto get_noteid() const noexcept -> dmn::note_id { return noteid_; }

  [[nodiscard]] auto try_get_handle() const noexcept -> std::optional<handle_t> {
    return hdl_->try_get();
  }
  [[nodiscard]] auto get_handle() const -> handle_t { return hdl_ ? hdl_->get() : handle_t{}; }

 private:
  dmn::database db_;

  using managed_handle_t = dmn::uhandle<handle_t>;
  std::shared_ptr<managed_handle_t> hdl_;
  std::shared_ptr<dmn::note_lock> lock_;
  dmn::note_id noteid_;

  note(dmn::database db, dmn::note_id noteid, handle_t handle);

  /// Internal implementation used by `dmn::database`.
  static auto open(dmn::database db, std::string_view unid) -> std::optional<note>;
  /// Internal implementation used by `dmn::database`.
  static auto open(dmn::database db, dmn::note_id noteid) -> std::optional<note>;
  /// Internal implementation used by `dmn::database`.
  static auto create(dmn::database db) -> note;

  /// Internal implementation used by `dmn::note::get_item_value()`.
  [[nodiscard]] auto get_item_value_impl(const lmbcs::str& key) const -> std::optional<dmn::object>;
  /// Internal implementation used by `dmn::note::append_item_value()`.
  void append_item_value(
    const lmbcs::str& key, dmn::type type, const void* data, uint16_t size
  ) const;
  /// Internal implementation used by `dmn::note::append_item_value()`.
  static auto get_raw_item_type(std::type_index type) -> uint16_t;

  friend class database;
  friend class view;
};
}  // namespace dmn