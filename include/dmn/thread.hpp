#pragma once

#include <functional>
#include <thread>

#include "dmn/detail/thread_context.hpp"

namespace dmn {
class thread {
 public:
  explicit thread(std::move_only_function<void()> fn)
      : thread_([fn = std::move(fn)]() mutable {
          const detail::thread_context ctx{};
          std::invoke(fn);
        }) {}

  explicit thread(std::move_only_function<void(std::stop_token)> fn)
      : thread_([fn = std::move(fn)](std::stop_token token) mutable {
          const detail::thread_context ctx{};
          std::invoke(fn, token);
        }) {}

  [[nodiscard]] auto joinable() const noexcept -> bool { return thread_.joinable(); }
  [[nodiscard]] auto get_id() const noexcept -> std::jthread::id { return thread_.get_id(); }

  void request_stop() noexcept { thread_.request_stop(); }
  void detach() { thread_.detach(); }
  void join() { thread_.join(); }

 private:
  std::jthread thread_;
};
}  // namespace dmn