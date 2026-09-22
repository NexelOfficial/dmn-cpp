#include "dmn/type.hpp"

#include <domino/global.h>
#include <domino/nsf.h>

#include <utility>

using dmn::info;
using dmn::type;

namespace {
constexpr auto native_info(info value) noexcept -> uint16_t {
  constexpr static uint16_t INFO_MASK = 0x8000;
  return std::to_underlying(value) & ~INFO_MASK;
}
}  // namespace

// Computable data types
static_assert(std::to_underlying(type::error) == TYPE_ERROR);
static_assert(std::to_underlying(type::unavailable) == TYPE_UNAVAILABLE);
static_assert(std::to_underlying(type::text) == TYPE_TEXT);
static_assert(std::to_underlying(type::text_list) == TYPE_TEXT_LIST);
static_assert(std::to_underlying(type::rfc822_text) == TYPE_RFC822_TEXT);
static_assert(std::to_underlying(type::number) == TYPE_NUMBER);
static_assert(std::to_underlying(type::number_range) == TYPE_NUMBER_RANGE);
static_assert(std::to_underlying(type::time) == TYPE_TIME);
static_assert(std::to_underlying(type::time_range) == TYPE_TIME_RANGE);
static_assert(std::to_underlying(type::formula) == TYPE_FORMULA);
static_assert(std::to_underlying(type::userid) == TYPE_USERID);

// Non-computable data types
static_assert(std::to_underlying(type::invalid_or_unknown) == TYPE_INVALID_OR_UNKNOWN);
static_assert(std::to_underlying(type::composite) == TYPE_COMPOSITE);
static_assert(std::to_underlying(type::collation) == TYPE_COLLATION);
static_assert(std::to_underlying(type::object) == TYPE_OBJECT);
static_assert(std::to_underlying(type::noteref_list) == TYPE_NOTEREF_LIST);
static_assert(std::to_underlying(type::view_format) == TYPE_VIEW_FORMAT);
static_assert(std::to_underlying(type::icon) == TYPE_ICON);
static_assert(std::to_underlying(type::notelink_list) == TYPE_NOTELINK_LIST);
static_assert(std::to_underlying(type::signature) == TYPE_SIGNATURE);
static_assert(std::to_underlying(type::seal) == TYPE_SEAL);
static_assert(std::to_underlying(type::sealdata) == TYPE_SEALDATA);
static_assert(std::to_underlying(type::seal_list) == TYPE_SEAL_LIST);
static_assert(std::to_underlying(type::highlights) == TYPE_HIGHLIGHTS);
static_assert(std::to_underlying(type::worksheet_data) == TYPE_WORKSHEET_DATA);
static_assert(std::to_underlying(type::userdata) == TYPE_USERDATA);
static_assert(std::to_underlying(type::query) == TYPE_QUERY);
static_assert(std::to_underlying(type::action) == TYPE_ACTION);
static_assert(std::to_underlying(type::assistant_info) == TYPE_ASSISTANT_INFO);
static_assert(std::to_underlying(type::viewmap_dataset) == TYPE_VIEWMAP_DATASET);
static_assert(std::to_underlying(type::viewmap_layout) == TYPE_VIEWMAP_LAYOUT);
static_assert(std::to_underlying(type::lsobject) == TYPE_LSOBJECT);
static_assert(std::to_underlying(type::html) == TYPE_HTML);
static_assert(std::to_underlying(type::sched_list) == TYPE_SCHED_LIST);
static_assert(std::to_underlying(type::calendar_format) == TYPE_CALENDAR_FORMAT);
static_assert(std::to_underlying(type::mime_part) == TYPE_MIME_PART);
static_assert(std::to_underlying(type::seal2) == TYPE_SEAL2);

// Info types
static_assert(native_info(info::note_id) == _NOTE_ID);
static_assert(native_info(info::oid) == _NOTE_OID);
static_assert(native_info(info::unid) == _NOTE_OID);
static_assert(info::oid != info::unid);