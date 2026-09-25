#include "dmn/note.hpp"

#include <domino/global.h>
#include <domino/nsfnote.h>
#include <domino/stdnames.h>
#include <domino/nsfdb.h>
#include <domino/osmem.h>
#include <filesystem>
#include <utility>
#include <random>

#include "dmn/detail/thread_context.hpp"
#include "dmn/object.hpp"
#include "dmn/error.hpp"
#include "dmn/type.hpp"
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

auto note::open_impl() const -> std::optional<handle_t> {
  auto& store = detail::thread_context::current().get<dmn::note>();
  store.remove_stale();

  handle_t handle = {};
  const dmn::status result =
    NSFNoteOpen(state_->db.get_handle(), state_->note_id.value, 0, &handle);
  if (result.is_not_found()) {
    return std::nullopt;
  }
  result.throw_if_error("Failed to open note");

  detail::uhandle<handle_t> managed{handle, NSFNoteClose};
  return store.insert(state_, std::move(managed)).handle.get();
}

auto note::open(dmn::database db, dmn::note_id note_id) -> std::optional<note> {
  note nt({.db = std::move(db), .note_id = note_id});
  if (!nt.open_impl()) {
    return std::nullopt;
  }
  
  return {std::move(nt)};
}

auto note::open(dmn::database db, dmn::unid unid) -> std::optional<note> {
  handle_t handle = {};
  const dmn::status result = NSFNoteOpenByUNID(db.get_handle(), unid.as_raw_unid(), 0, &handle);
  result.throw_if_error("Failed to open note");

  dmn::note_id note_id{};
  NSFNoteGetInfo(handle, _NOTE_ID, note_id.data());
  return open(std::move(db), note_id);
}

auto note::create(dmn::database db) -> note {
  handle_t handle = {};
  const dmn::status result = NSFNoteCreate(db.get_handle(), &handle);
  result.throw_if_error("Failed to create note");

  uint16_t note_class = NOTE_CLASS_DOCUMENT;
  NSFNoteSetInfo(handle, _NOTE_CLASS, &note_class);

  detail::uhandle<handle_t> managed{handle, NSFNoteClose};
  return note({.db = std::move(db), .hdl = std::move(managed)});
}

auto note::has(std::string_view key) const -> bool {
  const auto converted = dmn::lmbcs::from_string(key);
  return NSFItemIsPresent(get_handle(), converted.c_str(), converted.size());
}

auto note::copy_to_database(dmn::database other) const -> std::optional<note> {
  dmn::note_id new_note_id{};
  const dmn::status result = NSFDbCopyNote(
    state_->db.get_handle(), nullptr, nullptr, info<dmn::info::note_id>().value, other.get_handle(),
    nullptr, nullptr, new_note_id.data(), nullptr
  );

  result.throw_if_error("Failed to copy note");
  return open(std::move(other), new_note_id);
}

void note::embed_element(std::string_view name, const std::filesystem::path& path) const {
  const auto conv_name = dmn::lmbcs::from_string(name);
  const auto conv_path = dmn::lmbcs::from_string(path.string());
  const dmn::status result = NSFNoteAttachFile(
    get_handle(), ITEM_NAME_ATTACHMENT, strlen(ITEM_NAME_ATTACHMENT), conv_path.c_str(),
    conv_name.c_str(), COMPRESS_LZ1 | HOST_LOCAL
  );
  result.throw_if_error("Failed to embed element");
}

void note::embed_element(const std::filesystem::path& path) const {
  constexpr static uint8_t ATTACHMENT_NAME_LEN = 5;
  constexpr static std::string_view RAND_CHARSET = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  static std::random_device rd{};
  static std::mt19937 gen{rd()};

  std::string rand_name;
  for (size_t i = 0; i < ATTACHMENT_NAME_LEN; ++i) {
    std::uniform_int_distribution<> distrib(0, RAND_CHARSET.size() - 1);
    rand_name += RAND_CHARSET.at(distrib(gen));
  }

  const auto ext = path.has_extension() ? path.extension().string() : "";
  embed_element("ATT" + rand_name + ext, path);
}

void note::compute_with_form() const {
  const dmn::status result =
    NSFNoteComputeWithForm(get_handle(), detail::dhandle_t{}, 0, cwf_callback, nullptr);
  result.throw_if_error("Failed to compute with form");
}

auto note::get_type(std::string_view key) const -> dmn::type {
  const auto converted = dmn::lmbcs::from_string(key);
  auto data_type = dmn::type::invalid_or_unknown;
  NSFItemInfo(
    get_handle(), converted.c_str(), converted.size(), nullptr,
    reinterpret_cast<uint16_t*>(&data_type), nullptr, nullptr
  );
  return data_type;
}

void note::erase(std::string_view key) const {
  const auto converted = dmn::lmbcs::from_string(key);
  const dmn::status result = NSFItemDelete(get_handle(), converted.c_str(), converted.size());
  result.throw_if_error("Failed to remove key");
}

void note::sign() const {
  const dmn::status result = NSFNoteSign(get_handle());
  result.throw_if_error("Failed to sign note");
}

void note::save(bool force) const {
  const dmn::status result = NSFNoteUpdate(get_handle(), force ? UPDATE_FORCE : 0);
  result.throw_if_error("Failed to save note");
  state_->note_id = info<info::note_id>();
  state_->hdl = std::nullopt;
}

void note::remove(bool force) const {
  auto& ctx = detail::thread_context::current();
  ctx.get<dmn::note>().remove_stale();

  const dmn::status result = NSFNoteDelete(
    state_->db.get_handle(), info<dmn::info::note_id>().value, force ? UPDATE_FORCE : 0
  );
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
    uint16_t name_len = 0;
    DWORD value_len = 0;

    dmn::lmbcs name;
    name.resize(MAX_FIELD_NAME_LEN);

    NSFItemQuery(
      hdl, item_bid, name.data(), name.size(), &name_len, nullptr, nullptr,
      reinterpret_cast<BLOCKID*>(&value_bid), &value_len
    );
    name.resize(name_len);

    auto converted = name.to_string();
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
  auto raw_info = std::to_underlying(key) & ~INFO_MASK;
  NSFNoteGetInfo(get_handle(), raw_info, out);
}

auto note::get_impl(dmn::lmbcs_view key) const -> std::optional<dmn::object> {
  detail::block_id item_bid{};
  uint16_t item_type = 0;
  detail::block_id value_bid{};
  DWORD value_len = 0;

  const dmn::status result = NSFItemInfo(
    get_handle(), key.data(), key.size(), reinterpret_cast<BLOCKID*>(&item_bid), &item_type,
    reinterpret_cast<BLOCKID*>(&value_bid), &value_len
  );

  if (result.is_not_found()) {
    return std::nullopt;
  }
  result.throw_if_error("Failed to get item on note");

  auto owner = std::make_shared<dmn::note>(*this);
  return dmn::object{value_bid, value_len, owner, item_bid};
}

void note::append_impl(
  std::string_view key, dmn::type type, std::span<const std::byte> buffer, uint16_t flags
) const {
  const auto converted = dmn::lmbcs::from_string(key);
  const auto data_type = std::to_underlying(type);
  flags |= get_flags(buffer.size());

  const dmn::status result = NSFItemAppend(
    get_handle(), flags, converted.c_str(), converted.size(), data_type, buffer.data(),
    buffer.size()
  );
  result.throw_if_error("Failed to append item value");
}

void note::modify_impl(
  std::string_view key, dmn::type type, std::span<const std::byte> buffer, uint16_t flags
) const {
  auto obj = get<dmn::object>(key);
  if (!obj || *obj->item_bid_ == detail::block_id{}) {
    throw dmn::invalid_argument("Provided key doesn't exist on note");
  }

  const auto bid = std::bit_cast<BLOCKID>(*obj->item_bid_);
  const auto data_type = std::to_underlying(type);
  flags |= get_flags(buffer.size());

  const dmn::status result =
    NSFItemModifyValue(get_handle(), bid, flags, data_type, buffer.data(), buffer.size());
  result.throw_if_error("Failed to modify note item value");
}

auto note::get_handle() const -> handle_t {
  if (state_->hdl) {
    return state_->hdl->get();
  }

  auto& store = detail::thread_context::current().get<dmn::note>();
  store.remove_stale();

  auto* entry = store.find(state_);
  if (entry != nullptr) {
    return entry->handle.get();
  }

  handle_t handle = {};
  const dmn::status result =
    NSFNoteOpen(state_->db.get_handle(), state_->note_id.value, 0, &handle);
  result.throw_if_error("Failed to open note");

  detail::uhandle<handle_t> managed{handle, NSFNoteClose};
  return store.insert(state_, std::move(managed)).handle.get();
}