#include "dmn/nsf/note.hpp"

#include <domino/global.h>
#include <domino/nsfnote.h>
#include <domino/stdnames.h>
#include <domino/nsfdb.h>
#include <domino/osmem.h>
#include <filesystem>
#include <random>

#include "dmn/misc/error.hpp"
#include "dmn/misc/unid.hpp"
#include "dmn/nos/list.hpp"
#include "dmn/nos/object.hpp"
#include "dmn/os/locker.hpp"

using dmn::note;

static_assert(sizeof(note::handle_t) == sizeof(NOTEHANDLE));

namespace {
struct scan_context {
  std::optional<std::regex> pattern;
  note::object_map_t objects;
  dmn::note note;
};

// NOLINTNEXTLINE(modernize-use-trailing-return-type)
STATUS LNCALLBACK cwf_callback(
  const void* /* unused */, WORD /* unused */, STATUS /* unused */, DHANDLE /* unused */,
  WORD /* unused */, void* /* unused */
) {
  return CWF_NEXT_FIELD;
}
}  // namespace

note::note(dmn::database db, dmn::note_id noteid, handle_t handle)
    : hdl_(std::make_shared<managed_handle_t>(handle, NSFNoteClose)),
      noteid_(noteid),
      db_(std::move(db)) {}

auto note::open(dmn::database db, dmn::note_id noteid) -> std::optional<note> {
  handle_t handle = {};
  const dmn::status result = NSFNoteOpen(db.get_handle(), noteid.value, 0, &handle);
  if (result.is_not_found()) {
    return std::nullopt;
  }
  result.throw_if_error("Failed to open note");

  return note(std::move(db), noteid, handle);
}

auto note::open(dmn::database db, std::string_view raw_unid) -> std::optional<note> {
  auto id = dmn::unid::from_string(raw_unid);
  if (!id) {
    return std::nullopt;
  }

  // Open note with it's handle
  handle_t handle = {};
  const dmn::status result = NSFNoteOpenByUNID(db.get_handle(), id->as_raw_unid(), 0, &handle);
  if (result.is_not_found()) {
    return std::nullopt;
  }
  result.throw_if_error("Failed to open note");

  dmn::note_id noteid{};
  NSFNoteGetInfo(handle, _NOTE_ID, noteid.data());

  return note(std::move(db), noteid, handle);
}

auto note::create(dmn::database db) -> note {
  handle_t handle = {};
  dmn::status result = NSFNoteCreate(db.get_handle(), &handle);
  result.throw_if_error("Failed to create note");

  uint16_t note_class = NOTE_CLASS_DOCUMENT;
  NSFNoteSetInfo(handle, _NOTE_CLASS, &note_class);

  result = NSFNoteUpdate(handle, 0);
  result.throw_if_error("Failed to save new note");

  dmn::note_id noteid{};
  NSFNoteGetInfo(handle, _NOTE_ID, noteid.data());

  return {std::move(db), noteid, handle};
}

auto note::has_item(std::string_view key) const -> bool {
  const lmbcs::str converted = lmbcs::translate(key);
  return NSFItemIsPresent(get_handle(), lmbcs::cast(converted), converted.size());
}

auto note::copy_to_database(const dmn::database& db) const -> std::optional<note> {
  dmn::note_id new_noteid{};
  const dmn::status result = NSFDbCopyNote(
    db_.get_handle(), nullptr, nullptr, get_noteid().value, db.get_handle(), nullptr, nullptr,
    new_noteid.data(), nullptr
  );

  result.throw_if_error("Failed to copy note");
  if (new_noteid == dmn::note_id{}) {
    throw dmn::error("Failed to copy note");
  }

  return note::open(db, new_noteid);
}

void note::append_item_value(
  const lmbcs::str& key, dmn::type type, const void* data, uint16_t size
) const {
  const bool is_summary = size < MAXONESEGSIZE / 4;
  const auto data_type = static_cast<uint16_t>(type);
  const dmn::status result = NSFItemAppend(
    get_handle(), is_summary ? ITEM_SUMMARY : 0, lmbcs::cast(key), key.size(), data_type, data, size
  );
  result.throw_if_error("Failed to append item value");
}

void note::embed_element(const std::string& name, const std::string& path) const {
  const lmbcs::str conv_name = lmbcs::translate(name);
  const lmbcs::str conv_path = lmbcs::translate(path);
  const dmn::status result = NSFNoteAttachFile(
    get_handle(), ITEM_NAME_ATTACHMENT, strlen(ITEM_NAME_ATTACHMENT), lmbcs::cast(conv_path),
    lmbcs::cast(conv_name), COMPRESS_LZ1 | HOST_LOCAL
  );
  result.throw_if_error("Failed to embed element");
}

void note::embed_element(const std::string& path) const {
  constexpr static uint8_t ATTACHMENT_NAME_LEN = 5;
  constexpr static std::string_view RAND_CHARSET = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  static std::random_device rd{};
  static std::mt19937 gen{rd()};

  std::string rand_name;
  for (size_t i = 0; i < ATTACHMENT_NAME_LEN; ++i) {
    std::uniform_int_distribution<> distrib(0, RAND_CHARSET.size() - 1);
    rand_name += RAND_CHARSET.at(distrib(gen));
  }

  const std::filesystem::path file_path{path};
  const auto ext = file_path.has_extension() ? file_path.extension().string() : "";

  embed_element("ATT" + rand_name + ext, path);
}

void note::compute_with_form() const {
  const dmn::status result =
    NSFNoteComputeWithForm(get_handle(), dmn::dhandle_t{}, 0, cwf_callback, nullptr);
  result.throw_if_error("Failed to compute with form");
}

void note::remove_item(std::string_view key) const {
  const lmbcs::str converted = lmbcs::translate(key);
  const dmn::status result = NSFItemDelete(get_handle(), lmbcs::cast(converted), converted.size());
  result.throw_if_error("Failed to remove key");
}

void note::save(bool force) const {
  const dmn::status result = NSFNoteUpdate(get_handle(), force ? UPDATE_FORCE : 0);
  result.throw_if_error("Failed to save note");
}

void note::remove(bool force) const {
  const dmn::status result =
    NSFNoteDelete(db_.get_handle(), get_noteid().value, force ? UPDATE_FORCE : 0);
  result.throw_if_error("Failed to remove note");
}

void note::lock() {
  if (lock_) {
    return;
  }

  lock_ = dmn::note_lock::acquire(db_, noteid_);
}

auto note::try_lock() noexcept -> bool {
  if (lock_) {
    return true;
  }

  auto lock = dmn::note_lock::try_acquire(db_, noteid_);
  if (!lock) {
    return false;
  }
  lock_ = std::move(lock);
  return true;
}

void note::unlock() {
  if (!lock_) {
    return;
  }

  lock_->unlock();
  lock_.reset();
}

auto note::try_unlock() noexcept -> bool {
  if (!lock_) {
    return true;
  }

  const bool success = lock_->try_unlock();
  if (success) {
    lock_.reset();
  }
  return success;
}

auto note::items(std::optional<std::regex> pattern) const -> object_map_t {
  constexpr static uint16_t MAX_FIELD_NAME_LEN = 64;
  const auto hdl = get_handle();

  BLOCKID item_bid{};
  dmn::status result = NSFItemInfo(hdl, nullptr, 0, &item_bid, nullptr, nullptr, nullptr);
  result.throw_if_error("Failed to iterate over note items");

  object_map_t output{};
  while (!result.is_not_found()) {
    dmn::os::block_id value_bid{};
    DWORD value_len = 0;

    lmbcs::str name(MAX_FIELD_NAME_LEN, '\0');
    uint16_t name_len = 0;

    NSFItemQuery(
      hdl, item_bid, lmbcs::cast(name), name.size(), &name_len, nullptr, nullptr,
      reinterpret_cast<BLOCKID*>(&value_bid), &value_len
    );

    name.resize(name_len);
    std::string converted = lmbcs::translate(name);
    auto owner = std::make_shared<dmn::note>(*this);
    auto obj = dmn::object{value_bid, value_len, owner};

    if (!pattern || std::regex_match(converted, *pattern)) {
      output.emplace(std::move(converted), std::move(obj));
    }

    BLOCKID next_item{};
    result = NSFItemInfoNext(hdl, item_bid, nullptr, 0, &next_item, nullptr, nullptr, nullptr);
    if (result.is_error() && !result.is_not_found()) {
      result.throw_if_error("Failed to iterate over note items");
    }

    item_bid = next_item;
  }

  return output;
}

auto note::get_universalid() const -> std::string {
  dmn::oid id{};
  NSFNoteGetInfo(get_handle(), _NOTE_OID, &id);
  return id.universalid.to_string();
}

auto note::get_item_value_impl(const lmbcs::str& key) const -> std::optional<dmn::object> {
  dmn::os::block_id value_bid{};
  DWORD value_len = 0;
  BLOCKID item_bid;
  uint16_t item_type = 0;

  const dmn::status result = NSFItemInfo(
    get_handle(), lmbcs::cast(key), key.size(), &item_bid, &item_type,
    reinterpret_cast<BLOCKID*>(&value_bid), &value_len
  );

  if (result.is_not_found()) {
    return std::nullopt;
  }
  result.throw_if_error("Failed to get item on note");

  auto owner = std::make_shared<dmn::note>(*this);
  return dmn::object{value_bid, value_len, owner};
}