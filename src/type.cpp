#include "dmn/type.hpp"

#include <domino/global.h>
#include <domino/nsf.h>

using dmn::info;
using dmn::type;

namespace {
constexpr auto native_type(type value) noexcept -> uint16_t { return static_cast<uint16_t>(value); }

constexpr auto native_info(info value) noexcept -> uint16_t {
  constexpr static uint16_t INFO_MASK = 0x8000;
  return static_cast<uint16_t>(value) & ~INFO_MASK;
}
}  // namespace

// Computable data types
static_assert(native_type(type::error) == TYPE_ERROR);
static_assert(native_type(type::unavailable) == TYPE_UNAVAILABLE);
static_assert(native_type(type::text) == TYPE_TEXT);
static_assert(native_type(type::text_list) == TYPE_TEXT_LIST);
static_assert(native_type(type::rfc822_text) == TYPE_RFC822_TEXT);
static_assert(native_type(type::number) == TYPE_NUMBER);
static_assert(native_type(type::number_range) == TYPE_NUMBER_RANGE);
static_assert(native_type(type::time) == TYPE_TIME);
static_assert(native_type(type::time_range) == TYPE_TIME_RANGE);
static_assert(native_type(type::formula) == TYPE_FORMULA);
static_assert(native_type(type::userid) == TYPE_USERID);

// Non-computable data types
static_assert(native_type(type::invalid_or_unknown) == TYPE_INVALID_OR_UNKNOWN);
static_assert(native_type(type::composite) == TYPE_COMPOSITE);
static_assert(native_type(type::collation) == TYPE_COLLATION);
static_assert(native_type(type::object) == TYPE_OBJECT);
static_assert(native_type(type::noteref_list) == TYPE_NOTEREF_LIST);
static_assert(native_type(type::view_format) == TYPE_VIEW_FORMAT);
static_assert(native_type(type::icon) == TYPE_ICON);
static_assert(native_type(type::notelink_list) == TYPE_NOTELINK_LIST);
static_assert(native_type(type::signature) == TYPE_SIGNATURE);
static_assert(native_type(type::seal) == TYPE_SEAL);
static_assert(native_type(type::sealdata) == TYPE_SEALDATA);
static_assert(native_type(type::seal_list) == TYPE_SEAL_LIST);
static_assert(native_type(type::highlights) == TYPE_HIGHLIGHTS);
static_assert(native_type(type::worksheet_data) == TYPE_WORKSHEET_DATA);
static_assert(native_type(type::userdata) == TYPE_USERDATA);
static_assert(native_type(type::query) == TYPE_QUERY);
static_assert(native_type(type::action) == TYPE_ACTION);
static_assert(native_type(type::assistant_info) == TYPE_ASSISTANT_INFO);
static_assert(native_type(type::viewmap_dataset) == TYPE_VIEWMAP_DATASET);
static_assert(native_type(type::viewmap_layout) == TYPE_VIEWMAP_LAYOUT);
static_assert(native_type(type::lsobject) == TYPE_LSOBJECT);
static_assert(native_type(type::html) == TYPE_HTML);
static_assert(native_type(type::sched_list) == TYPE_SCHED_LIST);
static_assert(native_type(type::calendar_format) == TYPE_CALENDAR_FORMAT);
static_assert(native_type(type::mime_part) == TYPE_MIME_PART);
static_assert(native_type(type::seal2) == TYPE_SEAL2);

// Info types
static_assert(native_info(info::note_id) == _NOTE_ID);
static_assert(native_info(info::oid) == _NOTE_OID);
static_assert(native_info(info::unid) == _NOTE_OID);
static_assert(info::oid != info::unid);