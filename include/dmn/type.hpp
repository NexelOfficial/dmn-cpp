#pragma once

#include <cstdint>

namespace dmn {
enum class type : uint16_t {
  // Computable data types
  error = (1U << 8),
  unavailable = (2U << 8),
  text = (5U << 8),
  text_list = (5U << 8) + 1U,
  rfc822_text = (5U << 8) + 2U,
  number = (3U << 8),
  number_range = (3U << 8) + 1U,
  time = (4U << 8),
  time_range = (4U << 8) + 1U,
  formula = (6U << 8),
  userid = (7U << 8),

  // Non-computable data types
  invalid_or_unknown = 0U,
  composite = 1U,
  collation = 2U,
  object = 3U,
  noteref_list = 4U,
  view_format = 5U,
  icon = 6U,
  notelink_list = 7U,
  signature = 8U,
  seal = 9U,
  sealdata = 10U,
  seal_list = 11U,
  highlights = 12U,
  worksheet_data = 13U,
  userdata = 14U,
  query = 15U,
  action = 16U,
  assistant_info = 17U,
  viewmap_dataset = 18U,
  viewmap_layout = 19U,
  lsobject = 20U,
  html = 21U,
  sched_list = 22U,
  calendar_format = 24U,
  mime_part = 25U,
  seal2 = 31U
};

constexpr auto is_canonical(dmn::type typ) -> bool {
  return typ != type::error && typ != type::unavailable && typ != type::text &&
         typ != type::text_list && typ != type::rfc822_text && typ != type::number &&
         typ != type::number_range && typ != type::time && typ != type::time_range &&
         typ != type::formula && typ != type::userid && typ != type::invalid_or_unknown &&
         typ != type::noteref_list && typ != type::notelink_list && typ != type::highlights &&
         typ != type::sched_list && typ != type::mime_part;
}

enum class info : uint16_t {
  note_id = 1U,
  oid = 2U,
  unid = 2U | 0x8000U,
};
}  // namespace dmn