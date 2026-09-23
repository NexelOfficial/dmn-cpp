#include "dmn/design/agent.hpp"

#include <domino/global.h>
#include <domino/queryods.h>
#include <domino/nsfobjec.h>
#include <domino/agents.h>
#include <domino/nif.h>
#include <chrono>
#include <utility>

#include "dmn/design/flags.hpp"
#include "dmn/detail/attached_object.hpp"
#include "dmn/detail/data_types.hpp"
#include "dmn/detail/ods.hpp"
#include "dmn/error.hpp"
#include "dmn/time_date.hpp"

template <>
struct dmn::detail::note_value<ODS_ASSISTSTRUCT> {
  static void apply(const auto& value, auto setter) {
    const auto size = ods::size(ods::type::ods_assiststruct);
    std::vector<std::byte> buffer{size};
    ods::write(buffer.data(), &value, ods::type::ods_assiststruct);
    std::invoke(setter, dmn::type::assistant_info, buffer);
  }
};

template <>
struct dmn::detail::note_value<CDQUERYHEADER> {
  static void apply(const auto& value, auto setter) {
    const auto size = ods::size(ods::type::cdqueryheader);
    std::vector<std::byte> buffer{size};
    ods::write(buffer.data(), &value, ods::type::cdqueryheader);
    std::invoke(setter, dmn::type::query, buffer);
  }
};

template <>
struct dmn::detail::note_value<dmn::design::flags> {
  static void apply(const auto& value, auto setter) {
    note_value<std::string>::apply(value.to_string(), std::move(setter));
  }
};

namespace ods = dmn::detail::ods;
namespace detail = dmn::detail;
using dmn::design::agent;

namespace {
auto get_action_item_impl(uint16_t code_size, ods::type action_header) -> detail::locker {
  auto size =
    ods::size(ods::type::cdactionheader) + ods::size(action_header) + code_size + sizeof(dmn::type);

  const auto length = static_cast<uint8_t>(ods::size(ods::type::cdactionheader));
  const BSIG header{.Signature = SIG_ACTION_HEADER, .Length = length};
  const CDACTIONHEADER action{.Header = header};

  auto lock = detail::locker::allocate(size);
  lock.write(dmn::type::action);
  lock.write(action, ods::type::cdactionheader);
  return lock;
}
}  // namespace

agent::agent(dmn::note note) : note_(std::move(note)) {};

auto agent::create(const dmn::database& db, std::string_view title) -> agent {
  const auto converted = dmn::lmbcs::from_string(title);
  const dmn::status result =
    NIFFindDesignNote(db.get_handle(), converted.c_str(), NOTE_CLASS_FILTER, nullptr);
  if (!result.is_not_found()) {
    result.throw_if_error("Failed to check for existing agent design");
    throw dmn::runtime_error("Agent design already exists");
  }

  auto note = db.create_note();
  uint16_t note_class = NOTE_CLASS_FILTER;
  NSFNoteSetInfo(note.get_handle(), _NOTE_CLASS, &note_class);

  auto assist_flags = std::string{ASSIST_FLAG_ENABLED} + std::string{ASSIST_FLAG_AGENT_RUNASSIGNER};
  note.set(ASSIST_FLAGS_ITEM, assist_flags, dmn::item_flag::sign);

  note.set(ASSIST_LASTRUN_ITEM, dmn::time_date{}, dmn::item_flag::sign);
  note.set(ASSIST_DOCCOUNT_ITEM, 0, dmn::item_flag::sign);
  note.set("$Generator", "dmn-cpp");

  agent ag(std::move(note));
  ag.set_title(title);
  ag.set_comment("");
  ag.set_trigger(trigger::none);
  ag.set_action_ex();
  ag.set_assist_query();
  ag.set_run_info();
  return ag;
}

auto agent::set_title(std::string_view title) -> agent& {
  note_.set(FIELD_TITLE, title, dmn::item_flag::sign);
  return *this;
}

auto agent::set_comment(std::string_view comment) -> agent& {
  note_.set(DESIGN_COMMENT, comment, dmn::item_flag::sign);
  return *this;
}

auto agent::set_code(dmn::lotusscript code) -> agent& {
  auto code_size = detail::checked_cast<uint16_t>(code.size());
  auto lock = get_action_item_impl(code_size, ods::type::cdactionlotusscript);

  const auto length = ods::size(ods::type::cdactionlotusscript) + code_size;
  const WSIG header{
    .Signature = SIG_ACTION_LOTUSSCRIPT, .Length = detail::checked_cast<uint16_t>(length)
  };
  const CDACTIONLOTUSSCRIPT action{.Header = header, .dwScriptLen = code_size};
  lock.write(action, ods::type::cdactionlotusscript);

  const auto cursor = code.get_cursor();
  lock.write(std::span{cursor.get_pointer(), cursor.size()});

  const dmn::object obj{std::move(lock)};
  note_.set(ASSIST_ACTION_ITEM, obj, dmn::item_flag::sign);
  note_.set(
    ASSIST_TYPE_ITEM, std::to_underlying(design::language::lotusscript), dmn::item_flag::sign
  );
  note_.set(DESIGN_FLAGS, flags::from_language(design::language::lotusscript));
  return *this;
}

auto agent::set_code(dmn::formula code) -> agent& {
  auto code_size = detail::checked_cast<uint16_t>(code.size(true));
  auto lock = get_action_item_impl(code_size, ods::type::cdactionformula);

  const auto length = ods::size(ods::type::cdactionformula) + code_size;
  const WSIG header{
    .Signature = SIG_ACTION_FORMULA, .Length = detail::checked_cast<uint16_t>(length)
  };
  const CDACTIONFORMULA action{.Header = header, .wFormulaLen = code_size};
  lock.write(action, ods::type::cdactionformula);

  const auto cursor = code.get_cursor();
  lock.write(std::span{cursor.get_pointer(), cursor.size()});

  const dmn::object obj{std::move(lock)};
  note_.set(ASSIST_ACTION_ITEM, obj, dmn::item_flag::sign);
  note_.set(ASSIST_TYPE_ITEM, std::to_underlying(design::language::formula), dmn::item_flag::sign);
  note_.set(DESIGN_FLAGS, flags::from_language(design::language::formula));
  return *this;
}

auto agent::set_trigger(design::trigger trig) -> agent& {
  auto raw_trig = std::to_underlying(trig);
  note_.set(ASSIST_TRIGGER_ITEM, std::to_string(raw_trig));

  const ODS_ASSISTSTRUCT assist{.wTriggerType = raw_trig, .wSearchType = 0};
  note_.set(ASSIST_INFO_ITEM, assist, dmn::item_flag::sign);
  return *this;
}

auto agent::get_title() const -> std::string {
  return note_.get<std::string>(FIELD_TITLE).value_or("");
}

auto agent::get_comment() const -> std::string {
  return note_.get<std::string>(DESIGN_COMMENT).value_or("");
}

void agent::save() {
  auto now = dmn::time_date::from_time_point(std::chrono::system_clock::now());
  note_.set(ASSIST_VERSION_ITEM, now);

  auto lang = design::language(note_.get<int>(ASSIST_TYPE_ITEM).value_or(0));
  if (lang == design::language::lotusscript) {
    const auto& db = note_.get_database();
    const dmn::status result = NSFNoteLSCompile(db.get_handle(), note_.get_handle(), 0);
    result.throw_if_error("Failed to compile agent LotusScript");
  }

  note_.sign();
  note_.save(false);
}

void agent::set_action_ex() {
  auto lock = detail::locker::allocate(4);
  lock.write(dmn::type::lsobject);

  const dmn::object obj{std::move(lock)};
  note_.set(ASSIST_EXACTION_ITEM, obj, dmn::item_flag::sign);
}

void agent::set_run_info() {
  const auto db = note_.get_database();
  const auto size = ods::size(ods::type::ods_assistrunobjectheader) +
                    ods::size(ods::type::ods_assistrunobjectentry);

  detail::attached_object object(db, detail::attached_object::type::assist_run_data, size);
  object.append_to_note(note_, ASSIST_RUNINFO_ITEM);

  const ODS_ASSISTRUNOBJECTHEADER run_header{.wEntries = 1};
  const ODS_ASSISTRUNOBJECTENTRY run_entry{};

  auto lock = detail::locker::allocate(size);
  lock.write(run_header, ods::type::ods_assistrunobjectheader);
  lock.write(run_entry, ods::type::ods_assistrunobjectentry);

  object.write(std::move(lock));
  (void)object.release();
}

void agent::set_assist_query() {
  const auto length = static_cast<uint8_t>(ods::size(ods::type::cdqueryheader));
  const BSIG header{.Signature = SIG_QUERY_HEADER, .Length = length};
  const CDQUERYHEADER query{.Header = header};
  note_.set(ASSIST_QUERY_ITEM, query, dmn::item_flag::sign);
}