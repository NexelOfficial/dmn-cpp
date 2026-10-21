#include "dmn/detail/runtime.hpp"

#include <domino/global.h>
#include <domino/kfm.h>

#include "dmn/detail/thread_context.hpp"
#include "dmn/error.hpp"

using dmn::detail::runtime;
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

auto runtime::get_username() -> std::string {
  session::instance();

  dmn::lmbcs name;
  name.resize(MAXUSERNAME + 1);

  const dmn::status result = SECKFMGetUserName(name.data());
  result.throw_if_error("Failed to determine username");

  name.resize(std::strlen(name.c_str()));
  return name.to_string();
}