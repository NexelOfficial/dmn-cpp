#include "dmn/view.hpp"

#include <domino/global.h>
#include <domino/nif.h>

#include <limits>
#include <optional>

#include "dmn/detail/locker.hpp"
#include "dmn/database.hpp"
#include "dmn/error.hpp"
#include "dmn/note.hpp"

using dmn::view;

static_assert(sizeof(view::handle_t) == sizeof(HCOLLECTION));

namespace {
struct key_buffer_data {
  ITEM_TABLE table;
  ITEM item;
  uint16_t type;
  // ...data goes here
};
}  // namespace

void view::iterate_entries(
  void* position_ptr, const query_options& opts, const function_t& func
) const {
  using namespace dmn::detail;

  uint16_t signal = SIGNAL_MORE_TO_DO;
  uint32_t remaining = opts.count;
  uint32_t current_skip = opts.offset;

  if (!opts.key && opts.offset != std::numeric_limits<uint32_t>::max()) {
    current_skip += 1;
  }

  while ((signal & SIGNAL_MORE_TO_DO) != 0 && remaining != 0) {
    dhandle_t entries_handle{};
    DWORD entries_length = 0;
    uint16_t memory_len = 0;

    const dmn::status result = NIFReadEntries(
      get_handle(), static_cast<COLLECTIONPOSITION*>(position_ptr), NAVIGATE_NEXT, current_skip,
      NAVIGATE_NEXT, remaining, READ_MASK_NOTEID | READ_MASK_SUMMARYVALUES, &entries_handle,
      &memory_len, nullptr, &entries_length, &signal
    );
    result.throw_if_error("Failed to read entries");

    auto entries_obj = locker(entries_handle, memory_len);
    if (entries_length == 0) {
      return;
    }

    // Read entries
    for (DWORD i = 0; i < entries_length; ++i) {
      // Read noteid
      entry current_entry{.noteid = entries_obj.read<NOTEID>()};

      // Read item table
      const size_t base_offset = entries_obj.get_offset();
      const auto item_table = entries_obj.read<ITEM_VALUE_TABLE>();

      // Get the length of each item in the table
      std::vector<uint16_t> item_lengths{};
      item_lengths.reserve(item_table.Items);
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
        const std::span buffer{entries_obj.get_pointer(), len};
        auto locker = locker::allocate<std::byte>(buffer);
        auto value = dmn::object{std::move(locker)};
        current_entry.columns.emplace_back(value);

        entries_obj.advance_offset(len);
      }

      entries_obj.set_offset(base_offset + item_table.Length);

      auto note = db_.get_note(current_entry.noteid);
      if (!note) {
        continue;
      }

      func(std::move(*note), current_entry.columns);
    }

    if (remaining <= entries_length) {
      return;
    }

    remaining -= entries_length;
    current_skip = 0;
  }
}

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
    db_handle, db_handle, view_noteid, 0, detail::dhandle_t{}, &handle, nullptr, nullptr, nullptr,
    nullptr
  );
  result.throw_if_error("Failed to open collection");

  return view(db, handle);
}

void view::iterate(const query_options& opts, const function_t& func) const {
  COLLECTIONPOSITION coll_pos{.Level = 0, .Tumbler = {0}};
  if (!opts.key) {
    iterate_entries(&coll_pos, opts, func);
    return;
  }

  DWORD num_matches = std::numeric_limits<uint32_t>::max();

  const lmbcs::str converted = lmbcs::translate(*opts.key);
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
    return;
  }
  result.throw_if_error("Failed to find entries by key");

  iterate_entries(&coll_pos, opts, func);
}

auto view::get_entries(const query_options& opts) const -> std::vector<entry> {
  std::vector<entry> entries{};
  iterate(opts, [&](const dmn::note& note, const std::vector<dmn::object>& columns) {
    entries.push_back(entry{.noteid = note.info<dmn::info::note_id>(), .columns = columns});
  });
  return entries;
}
