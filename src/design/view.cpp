#include "dmn/design/view.hpp"

#include <domino/global.h>
#include <domino/nif.h>
#include <domino/nifcoll.h>
#include <domino/nsfnote.h>
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

using dmn::design::view;

namespace {
auto make_item_name(uint16_t sequence) -> std::string { return "$" + std::to_string(sequence); }

auto build_collation() -> dmn::object {
  namespace detail = dmn::detail;
  namespace ods = detail::ods;

  COLLATION collation{};
  collation.BufferSize = ods::size(ods::type::collation) + ods::size(ods::type::collate_descriptor);
  collation.Items = 1;
  collation.signature = COLLATION_SIGNATURE;

  COLLATE_DESCRIPTOR descriptor{};
  descriptor.signature = COLLATE_DESCRIPTOR_SIGNATURE;
  descriptor.keytype = COLLATE_TYPE_NOTEID;

  auto locker = detail::locker::allocate(collation.BufferSize + sizeof(dmn::type));
  locker.write(dmn::type::collation);
  locker.write(collation, ods::type::collation);
  locker.write(descriptor, ods::type::collate_descriptor);
  return {std::move(locker)};
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

  note_.set(VIEW_VIEW_FORMAT_ITEM, view_format);
  note_.set(VIEW_COLLATION_ITEM, collation);
  note_.set(VIEW_FORMULA_ITEM, selection_);
  note_.save(false);
}

auto view::build_view_format() -> dmn::object {
  constexpr uint16_t DEFAULT_VIEW_WIDTH = 80;
  namespace ods = detail::ods;

  std::vector<lmbcs::str> item_names;
  std::vector<lmbcs::str> titles;
  std::vector<dmn::formula> formulas;
  std::vector<VIEW_COLUMN_FORMAT> columns;

  item_names.reserve(columns_.size());
  titles.reserve(columns_.size());
  formulas.reserve(columns_.size());
  columns.reserve(columns_.size());

  size_t buffer_size =
    ods::size(ods::type::view_table_format) + ods::size(ods::type::view_table_format2);

  for (auto& current : columns_) {
    if (current.formula_.empty()) {
      throw dmn::invalid_argument("View column formula is empty");
    }

    if (current.item_name_.empty()) {
      current.item_name_ = make_item_name(next_sequence_++);
    }

    auto item_name = lmbcs::translate(current.item_name_);
    buffer_size += item_name.size();
    item_names.emplace_back(std::move(item_name));

    auto title = lmbcs::translate(current.title_);
    buffer_size += title.size();
    titles.emplace_back(std::move(title));

    auto formula = dmn::formula::compile(current.formula_, current.item_name_);
    buffer_size += formula.size();
    formulas.emplace_back(std::move(formula));

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
    buffer_size +=
      ods::size(ods::type::view_column_format) + ods::size(ods::type::view_column_format2);
  }

  VIEW_TABLE_FORMAT table{};
  table.Header.Version = VIEW_FORMAT_VERSION;
  table.Header.ViewStyle = VIEW_STYLE_TABLE;
  table.Columns = columns.size();
  table.ItemSequenceNumber = next_sequence_;
  table.Flags = VIEW_TABLE_FLAG_CONFLICT;

  auto locker = detail::locker::allocate(buffer_size + sizeof(dmn::type));
  locker.write(dmn::type::view_format);
  locker.write(table, ods::type::view_table_format);

  for (const auto& current : columns) {
    locker.write(current, ods::type::view_column_format);
  }

  for (size_t i = 0; i < columns.size(); ++i) {
    const auto& item_name = item_names.at(i);
    const auto item_span = std::as_bytes(std::span{item_name.data(), item_name.size()});
    locker.write(item_span);
    selection_.add_summary(columns_.at(i).item_name_);

    const auto& title = titles.at(i);
    const auto title_span = std::as_bytes(std::span{title.data(), title.size()});
    locker.write(title_span);

    const auto& formula = formulas.at(i);
    const detail::locker cursor(formula.get_handle(), formula.size(), detail::ownership::borrow);
    const auto span = std::span{cursor.get_pointer(), cursor.size()};
    locker.write(span);
    selection_.merge(formula);
  }

  selection_.add_summary(VIEW_CONFLICT_ITEM);
  selection_.add_summary(FIELD_LINK);

  VIEW_TABLE_FORMAT2 table2{};
  table2.Length = detail::ods::size(ods::type::view_table_format2);
  table2.BackgroundColor = NOTES_COLOR_WHITE;
  table2.TitleFont = DEFAULT_BOLD_FONT_ID;
  table2.UnreadFont = DEFAULT_FONT_ID;
  table2.TotalsFont = DEFAULT_FONT_ID;
  table2.wSig = VALID_VIEW_FORMAT_SIG;
  locker.write(table2, ods::type::view_table_format2);

  for (size_t i = 0; i < columns.size(); ++i) {
    VIEW_COLUMN_FORMAT2 column2{};
    column2.Signature = VIEW_COLUMN_FORMAT_SIGNATURE2;
    locker.write(column2, ods::type::view_column_format2);
  }

  return {std::move(locker)};
}