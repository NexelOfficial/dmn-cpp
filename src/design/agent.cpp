#include "dmn/design/agent.hpp"

#include <domino/global.h>
#include <domino/queryods.h>
#include <domino/nsfobjec.h>
#include <domino/agents.h>
#include <domino/nif.h>
#include <chrono>
#include <array>

#include "dmn/design/flags.hpp"
#include "dmn/detail/ods.hpp"
#include "dmn/time_date.hpp"

namespace {
struct ls_object {
  std::array<std::byte, 2> pad;
};
}  // namespace

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
using dmn::design::agent;

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
  note.set(FIELD_TITLE, title, dmn::item_flag::sign);
  note.set(FILTER_COMMENT_ITEM, std::string{}, dmn::item_flag::sign);

  uint16_t note_class = NOTE_CLASS_FILTER;
  NSFNoteSetInfo(note.get_handle(), _NOTE_CLASS, &note_class);

  auto assist_flags = std::string{ASSIST_FLAG_ENABLED} + std::string{ASSIST_FLAG_AGENT_RUNASSIGNER};
  note.set(ASSIST_FLAGS_ITEM, assist_flags, dmn::item_flag::sign);

  const ODS_ASSISTSTRUCT assist{
    .wTriggerType = ASSISTTRIGGER_TYPE_MANUAL, .wSearchType = ASSISTSEARCH_TYPE_SELECTED
  };
  note.set(ASSIST_INFO_ITEM, assist, dmn::item_flag::sign);

  note.set(ASSIST_LASTRUN_ITEM, dmn::time_date{}, dmn::item_flag::sign);
  note.set(ASSIST_DOCCOUNT_ITEM, 0, dmn::item_flag::sign);
  note.set(ASSIST_TRIGGER_ITEM, std::to_string(ASSISTTRIGGER_TYPE_MANUAL));
  note.set("$Generator", "dmn-cpp");

  return {std::move(note)};
}

void agent::set_code(dmn::formula code) {
  auto formula_size = static_cast<uint16_t>(code.size(true));
  auto size = ods::size(ods::type::cdactionheader) + ods::size(ods::type::cdactionformula) +
              formula_size + sizeof(dmn::type);
  auto lock = detail::locker::allocate(size);
  lock.write(dmn::type::action);

  {
    const auto length = static_cast<uint8_t>(ods::size(ods::type::cdactionheader));
    const BSIG header{.Signature = SIG_ACTION_HEADER, .Length = length};
    const CDACTIONHEADER action{.Header = header};
    lock.write(action, ods::type::cdactionheader);
  }

  {
    const auto length = ods::size(ods::type::cdactionformula) + formula_size;
    const WSIG header{.Signature = SIG_ACTION_FORMULA, .Length = static_cast<uint16_t>(length)};
    const CDACTIONFORMULA action{.Header = header, .wFormulaLen = formula_size};
    lock.write(action, ods::type::cdactionformula);
  }

  {
    const auto cursor = code.get_cursor();
    const std::span span{cursor.get_pointer(), cursor.size()};
    lock.write(span);
  }

  const dmn::object obj{std::move(lock)};
  note_.set(ASSIST_ACTION_ITEM, obj, dmn::item_flag::sign);
  note_.set(
    ASSIST_TYPE_ITEM, static_cast<uint16_t>(design::language::formula), dmn::item_flag::sign
  );
  note_.set(DESIGN_FLAGS, flags::from_language(design::language::formula));
}

void agent::save() {
  auto now = dmn::time_date::from_time_point(std::chrono::system_clock::now());
  note_.set(ASSIST_VERSION_ITEM, now);

  set_action_ex();
  set_assist_query();
  set_run_info();

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
  // TODO: Fix object leak (NSFDbFreeObject) with DbObject wrapper
  DWORD object_id = 0;
  const auto db = note_.get_database();
  const auto object_size = ods::size(ods::type::ods_assistrunobjectheader) +
                           ods::size(ods::type::ods_assistrunobjectentry);

  {
    const dmn::status result =
      NSFDbAllocObject(db.get_handle(), object_size, NOTE_CLASS_DOCUMENT, 0, &object_id);
    result.throw_if_error("Failed to allocate object for agent");
  }

  {
    const auto item_size = sizeof(dmn::type) + ods::size(ods::type::object_descriptor);

    auto lock = detail::locker::allocate(item_size);
    lock.write(dmn::type::object);

    const OBJECT_DESCRIPTOR desc{.ObjectType = OBJECT_ASSIST_RUNDATA, .RRV = object_id};
    lock.write(desc, ods::type::object_descriptor);

    const BLOCKID bid{.pool = lock.get_handle(), .block = 0};
    const dmn::status result = NSFItemAppendObject(
      note_.get_handle(), ITEM_SUMMARY, ASSIST_RUNINFO_ITEM, std::strlen(ASSIST_RUNINFO_ITEM), bid,
      lock.size(), TRUE
    );
    result.throw_if_error("Failed to append info object to agent note");
    (void)lock.release();
  }

  {
    auto lock = detail::locker::allocate(object_size);

    const ODS_ASSISTRUNOBJECTHEADER run_header{.wEntries = 1};
    lock.write(run_header, ods::type::ods_assistrunobjectheader);

    const ODS_ASSISTRUNOBJECTENTRY run_entry{};
    lock.write(run_entry, ods::type::ods_assistrunobjectentry);

    const dmn::status result =
      NSFDbWriteObject(db.get_handle(), object_id, lock.get_handle(), 0, lock.size());
    result.throw_if_error("Failed to write object data to agent info object");
  }
}

void agent::set_assist_query() {
  const auto length = static_cast<uint8_t>(ods::size(ods::type::cdqueryheader));
  const BSIG header{.Signature = SIG_QUERY_HEADER, .Length = length};
  const CDQUERYHEADER query{.Header = header};
  note_.set(ASSIST_QUERY_ITEM, query, dmn::item_flag::sign);
}