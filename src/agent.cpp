#include "dmn/agent.hpp"

#include <domino/global.h>
#include <domino/agents.h>
#include <domino/nif.h>

#include "dmn/database.hpp"
#include "dmn/error.hpp"
#include "dmn/detail/lmbcs.hpp"
#include "dmn/detail/locker.hpp"

using dmn::agent;

static_assert(sizeof(HAGENT) == sizeof(agent::handle_t));

constexpr uint8_t ATTACHMENT_NAME_LEN = 5;
constexpr uint16_t MAX_TEXT_BUFFER = 60 * 1024;

agent::agent(dmn::database db, handle_t handle) : hdl_(handle, AgentClose), db_(std::move(db)) {}

auto agent::open(const dmn::database& db, std::string_view name) -> std::optional<agent> {
  const dmn::database::handle_t db_handle = db.get_handle();
  const lmbcs::str converted = lmbcs::translate(name);

  dmn::note_id agent_id{};
  dmn::status result =
    NIFFindDesignNote(db_handle, lmbcs::cast(converted), NOTE_CLASS_FILTER, agent_id.data());
  if (result.is_error()) {
    result = NIFFindPrivateDesignNote(
      db_handle, lmbcs::cast(converted), NOTE_CLASS_FILTER, agent_id.data()
    );
    if (result.is_not_found()) {
      return std::nullopt;
    }

    result.throw_if_error("Failed to find Agent");
  }

  handle_t handle = {};
  result = AgentOpen(db_handle, agent_id.value, &handle);
  result.throw_if_error("Failed to open Agent");

  return agent(db, handle);
}

auto agent::run(const std::optional<dmn::note>& note) const -> std::optional<std::string> {
  handle_t agent_hdl = hdl_.get();
  HAGENTCTX agent_ctx = {};
  dmn::status result = AgentCreateRunContext(agent_hdl, nullptr, 0, &agent_ctx);
  result.throw_if_error("Failed to create run context");

  const detail::uhandle<HAGENTCTX> ctx(agent_ctx, AgentDestroyRunContext);

  // Set document context (if provided)
  if (note) {
    result = AgentSetDocumentContext(agent_ctx, note->get_handle());
    result.throw_if_error("Failed to set document context");
  }

  // Redirect output to memory
  result = AgentRedirectStdout(agent_ctx, AGENT_REDIR_MEMORY);
  result.throw_if_error("Failed to redirect output");

  // Actually run the agent
  result = AgentRun(agent_hdl, agent_ctx, detail::dhandle_t{}, 0);
  result.throw_if_error("Failed to run agent");

  // Grab the output
  DWORD out_len = 0;
  detail::dhandle_t out_hdl = {};
  AgentQueryStdoutBuffer(agent_ctx, &out_hdl, &out_len);

  if (out_len == 0) {
    return std::string{};
  }

  if (out_hdl == detail::dhandle_t{} || out_len > MAX_TEXT_BUFFER) {
    return std::nullopt;
  }

  try {
    auto out_obj = detail::locker(out_hdl, out_len, detail::ownership::borrow);
    auto view = lmbcs::view(out_obj.get_pointer<lmbcs::char_t>(), out_len);
    return lmbcs::translate(view);
  } catch (...) {
    return std::nullopt;
  }
}