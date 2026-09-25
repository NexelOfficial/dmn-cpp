#include "dmn/detail/runtime.hpp"

#include <domino/global.h>

#include "dmn/detail/thread_context.hpp"
#include "dmn/error.hpp"

using dmn::detail::session;
using dmn::detail::thread;

session::session() {
  const dmn::status result = NotesInit();
  result.throw_if_error("Failed to initialize Notes");
}

session::~session() { NotesTerm(); }

auto session::instance() -> const session& {
  const static session s;
  const static detail::thread_context ctx{};
  return s;
}

thread::thread() {
  const dmn::status result = NotesInitThread();
  result.throw_if_error("Failed to initialize Notes thread");
}

thread::~thread() { NotesTermThread(); }