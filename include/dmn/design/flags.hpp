#pragma once

#include <cstdint>
#include <string>
#include <array>

#include "dmn/detail/runtime.hpp"

namespace dmn::design {
constexpr static uint8_t MAX_DESIGN_FLAGS = 64;

// NOLINTNEXTLINE(performance-enum-size)
enum class trigger : uint16_t {
  none,
  scheduled,
  newmail,
  pasted,
  manual,
  docupdate,
  synchnewmail,
  event,
  serverstart
};

enum class language : uint16_t { formula = 65412, lotusscript = 65413, java = 65427 };

enum class flag : char {
  invalid = '\0',
  add = 'A',
  antifolder = 'a',
  background_filter = 'B',
  init_by_design_only = 'b',
  no_compose = 'C',
  calendar_view = 'c',
  no_query = 'D',
  default_design = 'd',
  mail_filter = 'E',
  public_antifolder = 'e',
  folder_view = 'F',
  v4_agent = 'f',
  file = 'g',
  javascript_library = 'h',
  image_resource = 'i',
  java_agent = 'J',
  java_agent_source = 'j',
  xsp_page = 'K',
  lotusscript_agent = 'L',
  deleted_docs = 'l',
  new_ = 'N',
  hide_from_notes = 'n',
  preserve = 'P',
  private_first_use = 'p',
  replace_special = 'R',
  propagate_no_change = 'r',
  script_lib = 's',
  view_categorized = 'T',
  database_script = 't',
  subform = 'U',
  agent_run_as_web_user = 'u',
  agent_run_as_invoker = 'u',
  private_in_db = 'V',
  image_well = 'v',
  webpage = 'W',
  hide_from_web = 'w',
  no_menu = 'Y',
  shared_actions = 'y',
  frameset = '#',
  java_resource = '@',
  stylesheet_resource = '=',
  webservice = '{',
  shared_column = '^',
  propfile = '2',
  hide_from_v3 = '3',
  readonly = '&',
  needs_refresh = '$',
  html_file = '>',
  query_view = '<',
  directory = '/',
  print_form = '?',
  hide_from_design_list = '~',
  composite_def = ':',
  xsp_cc = ';',
  js_server = '.',
  java_file = '['
};

class flags : private detail::runtime {
 public:
  [[nodiscard]] static auto from_language(language lang) noexcept -> flags;
  [[nodiscard]] auto add_flag(flag fg) noexcept -> bool;
  [[nodiscard]] auto to_string() const -> std::string;

 private:
  std::array<flag, MAX_DESIGN_FLAGS> buffer_{};
};
}  // namespace dmn::design