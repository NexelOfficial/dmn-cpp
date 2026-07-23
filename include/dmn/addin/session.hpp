#pragma once

namespace dmn {
struct session {
 public:
  session();
  ~session();

  session(const session&) = delete;
  auto operator=(const session&) = delete;
  session(session&&) = delete;
  auto operator=(session&&) = delete;

  static auto instance() -> session& {
    static session s;
    return s;
  }
};

struct thread {
 public:
  thread();
  ~thread();

  thread(const thread&) = delete;
  auto operator=(const thread&) = delete;
  thread(thread&&) = delete;
  auto operator=(thread&&) = delete;
};
}  // namespace dmn
