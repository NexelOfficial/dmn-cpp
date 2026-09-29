#pragma once

namespace dmn::detail {
struct session {
 public:
  session();
  ~session();

  session(const session&) = delete;
  auto operator=(const session&) = delete;
  session(session&&) = delete;
  auto operator=(session&&) = delete;

  static auto instance() -> const session&;
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

class runtime {
 protected:
  runtime() { session::instance(); }
};
}  // namespace dmn::detail
