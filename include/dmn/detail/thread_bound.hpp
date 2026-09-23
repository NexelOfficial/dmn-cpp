#pragma once

#include <functional>
#include <optional>
#include <thread>

#include "dmn/error.hpp"

namespace dmn::detail {

template <typename T>
class thread_bound {
 public:
  thread_bound() : owner_(std::this_thread::get_id()) {};
  thread_bound(T value) : value_(std::move(value)), owner_(std::this_thread::get_id()) {}

  auto try_get() noexcept -> std::optional<std::reference_wrapper<T>> {
    if (is_owner_thread()) {
      return std::ref(value_);
    }
    return std::nullopt;
  }

  auto try_get() const noexcept -> std::optional<std::reference_wrapper<const T>> {
    if (is_owner_thread()) {
      return std::ref(value_);
    }
    return std::nullopt;
  }

  auto get() -> T& {
    check_thread();
    return value_;
  }
  auto get() const -> const T& {
    check_thread();
    return value_;
  }

  auto operator*() -> T& { return get(); }
  auto operator*() const -> const T& { return get(); }

  [[nodiscard]] auto is_owner_thread() const noexcept -> bool {
    return owner_ == std::this_thread::get_id();
  }

 private:
  void check_thread() const {
    if (owner_ != std::this_thread::get_id()) {
      throw dmn::thread_error("Object accessed from non-owning thread");
    }
  }

  T value_;
  std::thread::id owner_;
};
}  // namespace dmn::detail