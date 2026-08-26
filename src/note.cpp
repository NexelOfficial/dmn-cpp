#include "dmn/note.hpp"

#include <domino/global.h>
#include <domino/nsfnote.h>
#include <domino/stdnames.h>
#include <domino/nsfdb.h>
#include <domino/osmem.h>
#include <filesystem>
#include <random>

#include "dmn/object.hpp"
#include "dmn/error.hpp"
#include "dmn/unid.hpp"

using dmn::note;

static_assert(sizeof(note::handle_t) == sizeof(NOTEHANDLE));

namespace {
struct scan_context {
  std::optional<std::regex> pattern;
  note::object_map_t objects;
  dmn::note current_note;
};

// NOLINTNEXTLINE(modernize-use-trailing-return-type)
STATUS LNCALLBACK cwf_callback(
  const void* /* unused */, WORD /* unused */, STATUS /* unused */, DHANDLE /* unused */,
  WORD /* unused */, void* /* unused */
) {
  return CWF_NEXT_FIELD;
}

auto get_flags(size_t size) -> uint16_t {
  const bool is_summary = size < MAXONESEGSIZE / 4;
  return is_summary ? ITEM_SUMMARY : 0;
}
}  // namespace

note::note(dmn::database db, handle_t handle)
    : hdl_(std::make_shared<managed_handle_t>(handle, NSFNoteClose)), db_(std::move(db)) {}

auto note::open(dmn::database db, dmn::note_id noteid) -> std::optional<note> {
  handle_t handle = {};
  const dmn::status result = NSFNoteOpen(db.get_handle(), noteid.value, 0, &handle);
  if (result.is_not_found()) {
    return std::nullopt;
  }
  result.throw_if_error("Failed to open note");

  return note(std::move(db), handle);
}

auto note::open(dmn::database db, dmn::unid unid) -> std::optional<note> {
  // Open note with it's handle
  handle_t handle = {};
  const dmn::status result = NSFNoteOpenByUNID(db.get_handle(), unid.as_raw_unid(), 0, &handle);
  if (result.is_not_found()) {
    return std::nullopt;
  }
  result.throw_if_error("Failed to open note");

  dmn::note_id noteid{};
  NSFNoteGetInfo(handle, _NOTE_ID, noteid.data());

  return note(std::move(db), handle);
}

auto note::create(dmn::database db) -> note {
  handle_t handle = {};
  const dmn::status result = NSFNoteCreate(db.get_handle(), &handle);
  result.throw_if_error("Failed to create note");

  uint16_t note_class = NOTE_CLASS_DOCUMENT;
  NSFNoteSetInfo(handle, _NOTE_CLASS, &note_class);

  return {std::move(db), handle};
}

auto note::has(std::string_view key) const -> bool {
  const lmbcs::str converted = lmbcs::translate(key);
  return NSFItemIsPresent(get_handle(), lmbcs::cast(converted), converted.size());
}

auto note::copy_to_database(const dmn::database& db) const -> std::optional<note> {
  dmn::note_id new_noteid{};
  const dmn::status result = NSFDbCopyNote(
    db_.get_handle(), nullptr, nullptr, info<dmn::info::note_id>().value, db.get_handle(), nullptr,
    nullptr, new_noteid.data(), nullptr
  );

  result.throw_if_error("Failed to copy note");
  if (new_noteid == dmn::note_id{}) {
    throw dmn::runtime_error("New note copy has empty note_id");
  }

  return note::open(db, new_noteid);
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
    NSFNoteComputeWithForm(get_handle(), detail::dhandle_t{}, 0, cwf_callback, nullptr);
  result.throw_if_error("Failed to compute with form");
}

auto note::get_type(std::string_view key) const -> dmn::type {
  const lmbcs::str converted = lmbcs::translate(key);
  auto data_type = dmn::type::invalid_or_unknown;
  NSFItemInfo(
    get_handle(), lmbcs::cast(converted), converted.size(), nullptr,
    reinterpret_cast<uint16_t*>(&data_type), nullptr, nullptr
  );
  return data_type;
}

void note::erase(std::string_view key) const {
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
    NSFNoteDelete(db_.get_handle(), info<dmn::info::note_id>().value, force ? UPDATE_FORCE : 0);
  result.throw_if_error("Failed to remove note");
}

auto note::items(std::optional<std::regex> pattern) const -> object_map_t {
  constexpr static uint16_t MAX_FIELD_NAME_LEN = 64;
  const auto hdl = get_handle();

  BLOCKID item_bid{};
  dmn::status result = NSFItemInfo(hdl, nullptr, 0, &item_bid, nullptr, nullptr, nullptr);
  result.throw_if_error("Failed to iterate over note items");

  object_map_t output{};
  while (!result.is_not_found()) {
    detail::block_id value_bid{};
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

void note::get_info_impl(dmn::info key, void* out) const {
  constexpr static uint16_t INFO_MASK = 0x8000;
  auto raw_info = static_cast<uint16_t>(key) & ~INFO_MASK;
  NSFNoteGetInfo(get_handle(), raw_info, out);
}

auto note::get_impl(const lmbcs::str& key) const -> std::optional<dmn::object> {
  detail::block_id item_bid{};
  uint16_t item_type = 0;
  detail::block_id value_bid{};
  DWORD value_len = 0;

  const dmn::status result = NSFItemInfo(
    get_handle(), lmbcs::cast(key), key.size(), reinterpret_cast<BLOCKID*>(&item_bid), &item_type,
    reinterpret_cast<BLOCKID*>(&value_bid), &value_len
  );

  if (result.is_not_found()) {
    return std::nullopt;
  }
  result.throw_if_error("Failed to get item on note");

  auto owner = std::make_shared<dmn::note>(*this);
  return dmn::object{value_bid, value_len, owner, item_bid};
}

void note::append_impl(std::string_view key, dmn::type type, std::span<const std::byte> buffer) const {
  const lmbcs::str converted = lmbcs::translate(key);
  const auto data_type = static_cast<uint16_t>(type);
  const auto size = buffer.size();

  const dmn::status result = NSFItemAppend(
    get_handle(), get_flags(size), lmbcs::cast(converted), converted.size(), data_type,
    buffer.data(), size
  );
  result.throw_if_error("Failed to append item value");
}

void note::modify_impl(std::string_view key, dmn::type type, std::span<const std::byte> buffer) const {
  auto existing = get<dmn::object>(key);
  if (!existing) {
    throw dmn::invalid_argument("Provided key doesn't exist on note");
  }

  existing->write(type, buffer);
}