#include "dmn/nsf/view.hpp"

#include <domino/global.h>
#include <domino/nif.h>

#include <limits>
#include <optional>

#include "dmn/nsf/database.hpp"
#include "dmn/nsf/note.hpp"
#include "dmn/misc/error.hpp"
#include "dmn/os/locker.hpp"

using dmn::view;

static_assert(sizeof(view::handle_t) == sizeof(HCOLLECTION));

struct view::key_buffer_data {
  ITEM_TABLE table;
  ITEM item;
  uint16_t type;
  // ...data goes here
};

view::view(dmn::database db, handle_t handle)
    : hdl_(handle, NIFCloseCollection), db_(std::move(db)) {}

auto view::open(const dmn::database& db, std::string_view view_name) -> std::optional<view> {
  const dmn::database::handle_t db_handle = db.get_handle();

  NOTEID view_noteid = 0;
  const lmbcs::str converted = lmbcs::translate(view_name);
  dmn::status result = NIFFindView(db_handle, lmbcs::cast(converted), &view_noteid);
  if (result.is_not_found()) {
    return std::nullopt;
  }
  result.throw_if_error("Failed to find view");

  handle_t handle = {};
  result = NIFOpenCollection(
    db_handle, db_handle, view_noteid, 0, dmn::dhandle_t{}, &handle, nullptr, nullptr, nullptr,
    nullptr
  );
  result.throw_if_error("Failed to open collection");

  return view(db, handle);
}

void view::iterate(std::string_view key, const function_t& func) const {
  auto entries = get_entries(key);
  for (auto& entry : entries) {
    auto note = db_.get_note(entry.noteid);
    if (!note) {
      continue;
    }

    func(std::move(*note), entry.columns);
  }
}

auto view::get_entries() const -> std::vector<entry> { return get_entries(""); }

auto view::get_entries(std::string_view key) const -> std::vector<entry> {
  COLLECTIONPOSITION coll_pos{.Level = 0, .Tumbler = {0}};
  DWORD num_matches = std::numeric_limits<uint32_t>::max();

  if (!key.empty()) {
    const lmbcs::str converted = lmbcs::translate(key);
    const size_t text_len = converted.size();
    const size_t total = sizeof(key_buffer_data) + text_len;
    std::vector<char> buf(total);

    const std::span<char> ptr(buf.data(), buf.size());

    // Write header
    const auto key_buffer_span = ptr.subspan(0, sizeof(key_buffer_data));
    auto* key_buffer = reinterpret_cast<key_buffer_data*>(key_buffer_span.data());
    key_buffer->table.Length = total;
    key_buffer->table.Items = 1;
    key_buffer->item.NameLength = 0;
    key_buffer->item.ValueLength = text_len + sizeof(uint16_t);
    key_buffer->type = TYPE_TEXT;

    // Write data
    const auto data_span = ptr.subspan(sizeof(key_buffer_data), text_len);
    memcpy(data_span.data(), converted.c_str(), text_len);

    // Start at the beginning of the key
    const dmn::status result =
      NIFFindByKey(get_handle(), buf.data(), FIND_UPDATE_IF_NOT_FOUND, &coll_pos, &num_matches);
    if (result.is_not_found()) {
      return {};
    }
    result.throw_if_error("Failed to find view");
  }

  return collect_entries(&coll_pos, num_matches, key.empty() ? 1 : 0);
}

auto view::collect_entries(void* position_ptr, uint32_t count, uint32_t skip_count) const
  -> std::vector<entry> {
  std::vector<entry> entries{};

  uint16_t signal = SIGNAL_MORE_TO_DO;
  while ((signal & SIGNAL_MORE_TO_DO) != 0) {
    dmn::dhandle_t entries_handle{};
    DWORD entries_length = NULL;
    uint16_t memory_len = NULL;

    const dmn::status result = NIFReadEntries(
      get_handle(), static_cast<COLLECTIONPOSITION*>(position_ptr), NAVIGATE_NEXT, skip_count,
      NAVIGATE_NEXT, count, READ_MASK_NOTEID | READ_MASK_SUMMARYVALUES, &entries_handle,
      &memory_len, nullptr, &entries_length, &signal
    );
    result.throw_if_error("Failed to read entries");

    // We must take ownership of the handle
    auto entries_obj = dmn::os::locker(entries_handle, memory_len);

    // Verify we have results
    if (entries_length == 0) {
      return entries;
    }

    // Read entries
    for (DWORD i = 0; i < entries_length; ++i) {
      // Read noteid
      entry current_entry{.noteid = entries_obj.read<NOTEID>()};

      // Read item table
      const size_t base_offset = entries_obj.get_offset();
      const auto item_table = entries_obj.read<ITEM_VALUE_TABLE>();

      // Get the length of each item in the table
      std::vector<uint16_t> item_lengths(item_table.Items);
      for (uint16_t j = 0; j < item_table.Items; j++) {
        item_lengths.emplace_back(entries_obj.read<uint16_t>());
      }

      // Get the items in the table
      for (uint16_t j = 0; j < item_table.Items; j++) {
        // Skip empty items
        const uint16_t len = item_lengths.at(j);
        if (len == 0) {
          continue;
        }

        // Copy memory to owned object
        const std::span<uint8_t> buffer{entries_obj.get_pointer(), len};
        auto locker = dmn::os::locker::allocate(buffer);
        if (locker) {
          const auto bid = locker->get_block_id();
          auto owner = std::make_shared<dmn::os::locker>(std::move(*locker));
          auto value = dmn::object{bid, len, owner};
          current_entry.columns.emplace_back(value);
        } else {
          current_entry.columns.emplace_back();
        }

        entries_obj.increment_offset(len);
      }

      entries_obj.move_offset(base_offset + item_table.Length);
      entries.push_back(current_entry);
    }

    count -= entries_length;
  }

  return entries;
}
