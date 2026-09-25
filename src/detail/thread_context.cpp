#include "dmn/detail/thread_context.hpp"

#include <domino/global.h>

using dmn::detail::thread_context;

namespace {
auto current_context() noexcept -> thread_context*& {
  static thread_local thread_context* current = nullptr;
  return current;
}

void set_current(thread_context* ctx) { current_context() = ctx; }
}  // namespace

thread_context::thread_context() {
  if (current_context() != nullptr) {
    throw dmn::thread_error("Domino thread context already exists");
  }
  set_current(this);
  NotesInitThread();
}

thread_context::~thread_context() noexcept {
  storage_.clear();
  set_current(nullptr);
  NotesTermThread();
}

auto thread_context::current() -> thread_context& {
  if (current_context() == nullptr) {
    throw dmn::thread_error("No Domino thread context exists on this thread");
  }

  return *current_context();
}