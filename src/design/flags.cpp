#include "dmn/design/flags.hpp"

#include <domino/global.h>
#include <domino/stdnames.h>
#include <domino/queryods.h>
#include <domino/ods.h>

using dmn::design::flag;
using dmn::design::flags;
using dmn::design::language;
using dmn::design::trigger;

static_assert(static_cast<uint16_t>(trigger::none) == ASSISTTRIGGER_TYPE_NONE);
static_assert(static_cast<uint16_t>(trigger::scheduled) == ASSISTTRIGGER_TYPE_SCHEDULED);
static_assert(static_cast<uint16_t>(trigger::newmail) == ASSISTTRIGGER_TYPE_NEWMAIL);
static_assert(static_cast<uint16_t>(trigger::pasted) == ASSISTTRIGGER_TYPE_PASTED);
static_assert(static_cast<uint16_t>(trigger::manual) == ASSISTTRIGGER_TYPE_MANUAL);
static_assert(static_cast<uint16_t>(trigger::docupdate) == ASSISTTRIGGER_TYPE_DOCUPDATE);
static_assert(static_cast<uint16_t>(trigger::synchnewmail) == ASSISTTRIGGER_TYPE_SYNCHNEWMAIL);
static_assert(static_cast<uint16_t>(trigger::event) == ASSISTTRIGGER_TYPE_EVENT);
static_assert(static_cast<uint16_t>(trigger::serverstart) == ASSISTTRIGGER_TYPE_SERVERSTART);

static_assert(static_cast<uint16_t>(language::formula) == SIG_ACTION_FORMULA);
static_assert(static_cast<uint16_t>(language::lotusscript) == SIG_ACTION_LOTUSSCRIPT);
static_assert(static_cast<uint16_t>(language::java) == SIG_ACTION_JAVAAGENT);

static_assert(static_cast<char>(flag::add) == DESIGN_FLAG_ADD);
static_assert(static_cast<char>(flag::antifolder) == DESIGN_FLAG_ANTIFOLDER);
static_assert(static_cast<char>(flag::background_filter) == DESIGN_FLAG_BACKGROUND_FILTER);
static_assert(static_cast<char>(flag::init_by_design_only) == DESIGN_FLAG_INITBYDESIGNONLY);
static_assert(static_cast<char>(flag::no_compose) == DESIGN_FLAG_NO_COMPOSE);
static_assert(static_cast<char>(flag::calendar_view) == DESIGN_FLAG_CALENDAR_VIEW);
static_assert(static_cast<char>(flag::no_query) == DESIGN_FLAG_NO_QUERY);
static_assert(static_cast<char>(flag::default_design) == DESIGN_FLAG_DEFAULT_DESIGN);
static_assert(static_cast<char>(flag::mail_filter) == DESIGN_FLAG_MAIL_FILTER);
static_assert(static_cast<char>(flag::public_antifolder) == DESIGN_FLAG_PUBLICANTIFOLDER);
static_assert(static_cast<char>(flag::folder_view) == DESIGN_FLAG_FOLDER_VIEW);
static_assert(static_cast<char>(flag::v4_agent) == DESIGN_FLAG_V4AGENT);
static_assert(static_cast<char>(flag::file) == DESIGN_FLAG_FILE);
static_assert(static_cast<char>(flag::javascript_library) == DESIGN_FLAG_JAVASCRIPT_LIBRARY);
static_assert(static_cast<char>(flag::image_resource) == DESIGN_FLAG_IMAGE_RESOURCE);
static_assert(static_cast<char>(flag::java_agent) == DESIGN_FLAG_JAVA_AGENT);
static_assert(static_cast<char>(flag::java_agent_source) == DESIGN_FLAG_JAVA_AGENT_WITH_SOURCE);
static_assert(static_cast<char>(flag::xsp_page) == DESIGN_FLAG_XSPPAGE);
static_assert(static_cast<char>(flag::lotusscript_agent) == DESIGN_FLAG_LOTUSSCRIPT_AGENT);
static_assert(static_cast<char>(flag::deleted_docs) == DESIGN_FLAG_DELETED_DOCS);
static_assert(static_cast<char>(flag::new_) == DESIGN_FLAG_NEW);
static_assert(static_cast<char>(flag::hide_from_notes) == DESIGN_FLAG_HIDE_FROM_NOTES);
static_assert(static_cast<char>(flag::preserve) == DESIGN_FLAG_PRESERVE);
static_assert(static_cast<char>(flag::private_first_use) == DESIGN_FLAG_PRIVATE_1STUSE);
static_assert(static_cast<char>(flag::replace_special) == DESIGN_FLAG_REPLACE_SPECIAL);
static_assert(static_cast<char>(flag::propagate_no_change) == DESIGN_FLAG_PROPAGATE_NOCHANGE);
static_assert(static_cast<char>(flag::script_lib) == DESIGN_FLAG_SCRIPTLIB);
static_assert(static_cast<char>(flag::view_categorized) == DESIGN_FLAG_VIEW_CATEGORIZED);
static_assert(static_cast<char>(flag::database_script) == DESIGN_FLAG_DATABASESCRIPT);
static_assert(static_cast<char>(flag::subform) == DESIGN_FLAG_SUBFORM);
static_assert(static_cast<char>(flag::agent_run_as_web_user) == DESIGN_FLAG_AGENT_RUNASWEBUSER);
static_assert(static_cast<char>(flag::agent_run_as_invoker) == DESIGN_FLAG_AGENT_RUNASINVOKER);
static_assert(static_cast<char>(flag::private_in_db) == DESIGN_FLAG_PRIVATE_IN_DB);
static_assert(static_cast<char>(flag::image_well) == DESIGN_FLAG_IMAGE_WELL);
static_assert(static_cast<char>(flag::webpage) == DESIGN_FLAG_WEBPAGE);
static_assert(static_cast<char>(flag::hide_from_web) == DESIGN_FLAG_HIDE_FROM_WEB);
static_assert(static_cast<char>(flag::no_menu) == DESIGN_FLAG_NO_MENU);
static_assert(static_cast<char>(flag::shared_actions) == DESIGN_FLAG_SACTIONS);
static_assert(static_cast<char>(flag::frameset) == DESIGN_FLAG_FRAMESET);
static_assert(static_cast<char>(flag::java_resource) == DESIGN_FLAG_JAVA_RESOURCE);
static_assert(static_cast<char>(flag::stylesheet_resource) == DESIGN_FLAG_STYLESHEET_RESOURCE);
static_assert(static_cast<char>(flag::webservice) == DESIGN_FLAG_WEBSERVICE);
static_assert(static_cast<char>(flag::shared_column) == DESIGN_FLAG_SHARED_COL);
static_assert(static_cast<char>(flag::propfile) == DESIGN_FLAG_PROPFILE);
static_assert(static_cast<char>(flag::hide_from_v3) == DESIGN_FLAG_HIDE_FROM_V3);
static_assert(static_cast<char>(flag::readonly) == DESIGN_FLAG_READONLY);
static_assert(static_cast<char>(flag::needs_refresh) == DESIGN_FLAG_NEEDSREFRESH);
static_assert(static_cast<char>(flag::html_file) == DESIGN_FLAG_HTMLFILE);
static_assert(static_cast<char>(flag::query_view) == DESIGN_FLAG_QUERYVIEW);
static_assert(static_cast<char>(flag::directory) == DESIGN_FLAG_DIRECTORY);
static_assert(static_cast<char>(flag::print_form) == DESIGN_FLAG_PRINTFORM);
static_assert(static_cast<char>(flag::hide_from_design_list) == DESIGN_FLAG_HIDEFROMDESIGNLIST);
static_assert(static_cast<char>(flag::composite_def) == DESIGN_FLAG_COMPOSITE_DEF);
static_assert(static_cast<char>(flag::xsp_cc) == DESIGN_FLAG_XSP_CC);
static_assert(static_cast<char>(flag::js_server) == DESIGN_FLAG_JS_SERVER);
static_assert(static_cast<char>(flag::java_file) == DESIGN_FLAG_JAVAFILE);

auto flags::from_language(language lang) noexcept -> flags {
  flags out{};
  // TODO: Check if mandatory
  (void)out.add_flag(flag::v4_agent);
  (void)out.add_flag(flag::hide_from_v3);

  switch (lang) {
    case language::lotusscript:
      // (void)out.add_flag(DESIGN_FLAG_V4BACKGROUND_MACRO);
      (void)out.add_flag(flag::lotusscript_agent);
      break;
    case language::java:
      (void)out.add_flag(flag::java_agent);
      break;
    default:
      break;
  }

  return out;
}

auto flags::add_flag(flag fg) noexcept -> bool {
  if (auto it = std::ranges::find(buffer_, flag::invalid); it != buffer_.end()) {
    *it = fg;
    return true;
  }
  return false;
}

auto flags::to_string() const -> std::string {
  const auto end = std::ranges::find(buffer_, flag::invalid);
  const auto* ptr = reinterpret_cast<const char*>(buffer_.data());
  const auto size = static_cast<size_t>(end - buffer_.begin());
  return {ptr, size};
}