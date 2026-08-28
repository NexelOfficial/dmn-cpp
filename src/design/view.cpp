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

#include "dmn/detail/locker.hpp"
#include "dmn/database.hpp"
#include "dmn/formula.hpp"
#include "dmn/error.hpp"
#include "dmn/lmbcs.hpp"
#include "dmn/note.hpp"
#include "dmn/type.hpp"

using dmn::design::view;
namespace ods = dmn::detail::ods;

static_assert(sizeof(dmn::design::view_table_format) == sizeof(VIEW_TABLE_FORMAT));
static_assert(alignof(dmn::design::view_table_format) == alignof(VIEW_TABLE_FORMAT));

static_assert(sizeof(dmn::design::view_table_format2) == sizeof(VIEW_TABLE_FORMAT2));
static_assert(alignof(dmn::design::view_table_format2) == alignof(VIEW_TABLE_FORMAT2));

namespace {
auto make_item_name(uint16_t sequence) -> dmn::lmbcs { return {"$" + std::to_string(sequence)}; }

auto build_collation() -> dmn::object {
  namespace detail = dmn::detail;

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

view::view(dmn::note note)
    : note_(std::move(note)),
      selection_(dmn::formula{"@All"}),
      table_format_{
        .header = {VIEW_FORMAT_VERSION, VIEW_STYLE_TABLE},
        .sequence_number = 1,
        .flags = VIEW_TABLE_FLAG_CONFLICT,
      },
      table_format2_{
        .length = ods::size(ods::type::view_table_format2),
        .background_color = design::color::white,
        .title_font = DEFAULT_BOLD_FONT_ID,
        .unread_font = DEFAULT_FONT_ID,
        .totals_font = DEFAULT_FONT_ID,
        .signature = VALID_VIEW_FORMAT_SIG,
      } {}

auto view::create(const dmn::database& db, std::string_view title) -> view {
  NOTEID noteid = 0;
  const auto converted = dmn::lmbcs::from_string(title);
  const dmn::status result = NIFFindDesignNoteExt(
    db.get_handle(), converted.c_str(), NOTE_CLASS_VIEW, DFLAGPAT_VIEW, &noteid, 0
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
  out.selection_ = dmn::formula{"@All"};
  return out;
}

auto view::column(std::string_view title) -> design::column& {
  const auto converted = dmn::lmbcs::from_string(title);
  const auto itr = std::ranges::find_if(columns_, [&](const auto& current) {
    return current.title_ == converted;
  });
  if (itr != columns_.end()) {
    return *itr;
  }

  columns_.emplace_back();
  return columns_.back();
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
  size_t buffer_size =
    ods::size(ods::type::view_table_format) + ods::size(ods::type::view_table_format2);

  for (auto& current : columns_) {
    if (current.formula_.size() == 0) {
      throw dmn::invalid_argument("View column formula is empty");
    }

    if (current.item_name_.empty()) {
      current.item_name_ = make_item_name(table_format_.sequence_number++);
    }

    current.formula_.add_item_name(current.item_name_);

    current.format_.formula_size_ = current.formula_.size();
    current.format_.item_name_size_ = current.item_name_.size();
    current.format_.title_size_ = current.title_.size();

    buffer_size += current.formula_.size() + current.item_name_.size() + current.title_.size() +
                   ods::size(ods::type::view_column_format) +
                   ods::size(ods::type::view_column_format2);
  }

  table_format_.columns = columns_.size();

  auto locker = detail::locker::allocate(buffer_size + sizeof(dmn::type));
  locker.write(dmn::type::view_format);
  locker.write(table_format_, ods::type::view_table_format);

  for (const auto& current : columns_) {
    locker.write(current.format_, ods::type::view_column_format);
  }

  for (const auto& current : columns_) {
    const auto& item_name = current.item_name_;
    const auto& title = current.title_;
    const auto& formula = current.formula_;
    const auto cursor = formula.get_cursor();

    locker.write(std::span{item_name.c_str(), item_name.size()});
    locker.write(std::span{title.c_str(), title.size()});
    locker.write(std::span{cursor.get_pointer(), cursor.size()});

    selection_.add_summary(item_name);
    selection_.merge(formula);
  }

  if ((table_format_.flags & VIEW_TABLE_FLAG_CONFLICT) != 0) {
    selection_.add_summary(VIEW_CONFLICT_ITEM);
  }

  if ((table_format_.flags & VIEW_TABLE_FLAG_FLATINDEX) == 0) {
    selection_.add_summary(FIELD_LINK);
  }

  locker.write(table_format2_, ods::type::view_table_format2);

  for (const auto& current : columns_) {
    locker.write(current.format2_, ods::type::view_column_format2);
  }

  return {std::move(locker)};
}