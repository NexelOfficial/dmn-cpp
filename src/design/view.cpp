#include "dmn/design/view.hpp"

#include <domino/global.h>
#include <domino/nif.h>
#include <domino/nifcoll.h>
#include <domino/nsfnote.h>
#include <domino/ods.h>
#include <domino/editods.h>
#include <domino/stdnames.h>
#include <domino/colorid.h>
#include <domino/viewfmt.h>

#include <algorithm>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "dmn/database.hpp"
#include "dmn/design/column.hpp"
#include "dmn/detail/lmbcs.hpp"
#include "dmn/detail/locker.hpp"
#include "dmn/error.hpp"
#include "dmn/formula.hpp"
#include "dmn/note.hpp"
#include "dmn/type.hpp"

using dmn::lmbcs;
using dmn::design::view;

namespace {
struct raw_item {
  dmn::type type;
  std::vector<uint8_t> data;
};

void set_raw_item(
  const dmn::note& note, std::string_view name, dmn::type type, const std::vector<uint8_t>& data
) {
  if (note.has(name)) {
    note.erase(name);
  }

  const lmbcs::str converted = lmbcs::translate(name);
  const dmn::status result = NSFItemAppend(
    note.get_handle(), 0, lmbcs::cast(converted), converted.size(), static_cast<WORD>(type),
    data.empty() ? nullptr : data.data(), data.size()
  );
  result.throw_if_error("Failed to write note item");
}

template <typename T>
void append_ods(std::vector<uint8_t>& buffer, WORD type, const T& value) {
  const auto offset = buffer.size();
  buffer.resize(offset + ODSLength(type));

  // NOLINTBEGIN
  void* target = buffer.data() + offset;
  ODSWriteMemory(&target, type, &value, 1);
  // NOLINTEND
}

auto make_item_name(uint16_t sequence) -> std::string { return "$" + std::to_string(sequence); }

auto build_collation() -> std::vector<uint8_t> {
  COLLATION collation{};
  collation.BufferSize = ODSLength(_COLLATION) + ODSLength(_COLLATE_DESCRIPTOR);
  collation.Items = 1;
  collation.signature = COLLATION_SIGNATURE;

  COLLATE_DESCRIPTOR descriptor{};
  descriptor.signature = COLLATE_DESCRIPTOR_SIGNATURE;
  descriptor.keytype = COLLATE_TYPE_NOTEID;

  std::vector<uint8_t> buffer;
  buffer.reserve(collation.BufferSize);
  append_ods(buffer, _COLLATION, collation);
  append_ods(buffer, _COLLATE_DESCRIPTOR, descriptor);

  return buffer;
}
}  // namespace

view::view(dmn::note note) : note_(std::move(note)), selection_(dmn::formula::compile("@All")) {}

auto view::create(const dmn::database& db, std::string_view title) -> view {
  NOTEID noteid = 0;
  const lmbcs::str converted = lmbcs::translate(title);
  const dmn::status result = NIFFindDesignNoteExt(
    db.get_handle(), lmbcs::cast(converted), NOTE_CLASS_VIEW, DFLAGPAT_VIEW, &noteid, 0
  );
  if (!result.is_not_found()) {
    result.throw_if_error("Failed to check for existing view design");
    throw dmn::runtime_error("View design already exists");
  }

  auto note = db.create_note();
  uint16_t note_class = NOTE_CLASS_VIEW;
  NSFNoteSetInfo(note.get_handle(), _NOTE_CLASS, &note_class);
  note.set(VIEW_TITLE_ITEM, std::string(title));
  note.set(DESIGN_FLAGS, "PY");

  view out{std::move(note)};
  out.selection_ = dmn::formula::compile("@All");
  return out;
}

auto view::column(std::string_view title) -> design::column& {
  const auto itr =
    std::ranges::find_if(columns_, [&](const auto& current) { return current.title_ == title; });
  if (itr != columns_.end()) {
    return *itr;
  }

  columns_.emplace_back();
  return columns_.at(columns_.size() - 1);
}

auto view::set_selection_formula(dmn::formula formula) -> view& {
  selection_ = std::move(formula);
  return *this;
}

void view::save() {
  const auto view_format = build_view_format();
  const auto collation = build_collation();

  if (note_.has(VIEW_FORMULA_TIME_ITEM)) {
    note_.erase(VIEW_FORMULA_TIME_ITEM);
  }

  set_raw_item(note_, VIEW_COLLATION_ITEM, dmn::type::collation, collation);
  set_raw_item(note_, VIEW_VIEW_FORMAT_ITEM, dmn::type::view_format, view_format);
  note_.set(VIEW_FORMULA_ITEM, selection_);
  note_.save(false);
}

auto view::build_view_format() -> std::vector<uint8_t> {
  constexpr uint16_t DEFAULT_VIEW_WIDTH = 80;

  std::vector<lmbcs::str> item_names;
  std::vector<lmbcs::str> titles;
  std::vector<dmn::formula> formulas;
  std::vector<VIEW_COLUMN_FORMAT> columns;

  item_names.reserve(columns_.size());
  titles.reserve(columns_.size());
  formulas.reserve(columns_.size());
  columns.reserve(columns_.size());

  for (auto& current : columns_) {
    if (current.formula_.empty()) {
      throw dmn::invalid_argument("View column formula is empty");
    }

    if (current.item_name_.empty()) {
      current.item_name_ = make_item_name(next_sequence_++);
    }

    item_names.emplace_back(lmbcs::translate(current.item_name_));
    titles.emplace_back(lmbcs::translate(current.title_));
    formulas.emplace_back(dmn::formula::compile(current.formula_, current.item_name_));

    VIEW_COLUMN_FORMAT format{};
    format.Signature = VIEW_COLUMN_FORMAT_SIGNATURE;
    format.DisplayWidth = DEFAULT_VIEW_WIDTH;
    format.FontID = DEFAULT_BOLD_FONT_ID;
    format.FormatDataType = VIEW_COL_TEXT;
    format.FormulaSize = formulas.back().size();
    format.ItemNameSize = item_names.back().size();
    format.TitleSize = titles.back().size();
    format.ConstantValueSize = 0;
    columns.emplace_back(format);
  }

  VIEW_TABLE_FORMAT table{};
  table.Header.Version = VIEW_FORMAT_VERSION;
  table.Header.ViewStyle = VIEW_STYLE_TABLE;
  table.Columns = columns.size();
  table.ItemSequenceNumber = next_sequence_;
  table.Flags = VIEW_TABLE_FLAG_CONFLICT;

  std::vector<uint8_t> buffer;
  append_ods(buffer, _VIEW_TABLE_FORMAT, table);

  for (const auto& current : columns) {
    append_ods(buffer, _VIEW_COLUMN_FORMAT, current);
  }

  for (size_t i = 0; i < columns.size(); ++i) {
    const auto& item_name = item_names.at(i);
    buffer.insert(buffer.end(), item_name.begin(), item_name.end());
    selection_.add_summary(columns_.at(i).item_name_);

    const auto& title = titles.at(i);
    buffer.insert(buffer.end(), title.begin(), title.end());

    const auto& formula = formulas.at(i);
    const dmn::detail::locker cursor(
      formula.get_handle(), formula.size(), detail::ownership::borrow
    );
    const auto span = std::span{cursor.get_pointer(), cursor.size()};
    buffer.insert(buffer.end(), span.begin(), span.end());
    selection_.merge(formula);
  }

  selection_.add_summary(VIEW_CONFLICT_ITEM);
  selection_.add_summary(FIELD_LINK);

  VIEW_TABLE_FORMAT2 table2{};
  table2.Length = ODSLength(_VIEW_TABLE_FORMAT2);
  table2.BackgroundColor = NOTES_COLOR_WHITE;
  table2.TitleFont = DEFAULT_BOLD_FONT_ID;
  table2.UnreadFont = DEFAULT_FONT_ID;
  table2.TotalsFont = DEFAULT_FONT_ID;
  table2.wSig = VALID_VIEW_FORMAT_SIG;
  append_ods(buffer, _VIEW_TABLE_FORMAT2, table2);

  for (size_t i = 0; i < columns.size(); ++i) {
    VIEW_COLUMN_FORMAT2 column2{};
    column2.Signature = VIEW_COLUMN_FORMAT_SIGNATURE2;
    append_ods(buffer, _VIEW_COLUMN_FORMAT2, column2);
  }

  return buffer;
}