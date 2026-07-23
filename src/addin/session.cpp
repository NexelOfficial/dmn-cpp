#include "dmn/addin/session.hpp"

#include <domino/global.h>
#include <domino/osmisc.h>

#include "dmn/misc/error.hpp"

using dmn::session;
using dmn::thread;

session::session() {
  const dmn::status result = NotesInit();
  result.throw_if_error("Failed to initialize Notes");
}

session::~session() { NotesTerm(); }


thread::thread() {
  const dmn::status result = NotesInitThread();
  result.throw_if_error("Failed to initialize Notes thread");
}

thread::~thread() { NotesTermThread(); }