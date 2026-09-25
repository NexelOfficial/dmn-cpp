#pragma once

#include <memory>
#include <typeindex>
#include <unordered_map>

#include "dmn/detail/uhandle.hpp"

namespace dmn::detail {
template <typename State, typename Handle>
struct thread_handle {
  std::weak_ptr<State> state;
  detail::uhandle<Handle> handle;

  thread_handle(const std::shared_ptr<State>& state, detail::uhandle<Handle>&& handle)
      : state(state), handle(std::move(handle)) {}
};

template <typename T>
  requires std::is_class_v<typename T::state> && std::is_arithmetic_v<typename T::handle_t>
class thread_handle_store {
 public:
  using entry_type = thread_handle<typename T::state, typename T::handle_t>;

  void remove_stale() noexcept {
    std::erase_if(handles_, [](const auto& item) { return item.second.state.expired(); });
  }

  [[nodiscard]] auto find(std::shared_ptr<typename T::state> state) -> entry_type* {
    const auto it = handles_.find(state.get());
    if (it == handles_.end()) {
      return nullptr;
    }

    if (const auto existing = it->second.state.lock(); existing == state) {
      return &it->second;
    }

    handles_.erase(it);
    return nullptr;
  }

  auto insert(
    std::shared_ptr<typename T::state> state, detail::uhandle<typename T::handle_t> handle
  ) -> entry_type& {
    auto [it, _] = handles_.try_emplace(state.get(), state, std::move(handle));
    return it->second;
  }

 private:
  std::unordered_map<typename T::state*, entry_type> handles_;
};

class thread_context {
  struct storage_base {
    virtual ~storage_base() = default;
  };

  template <typename T>
  struct storage final : storage_base {
    T value;
  };

 public:
  thread_context();
  ~thread_context() noexcept;

  thread_context(const thread_context&) = delete;
  auto operator=(const thread_context&) = delete;
  thread_context(thread_context&&) = delete;
  auto operator=(thread_context&&) = delete;

  [[nodiscard]] static auto current() -> thread_context&;

  template <typename T>
  [[nodiscard]] auto get() -> thread_handle_store<T>& {
    using storage_t = storage<thread_handle_store<T>>;
    const auto key = std::type_index(typeid(thread_handle_store<T>));
    auto [it, inserted] = storage_.try_emplace(key);
    if (inserted) {
      it->second = std::make_unique<storage_t>();
    }

    return static_cast<storage_t&>(*it->second).value;
  }

 private:
  std::unordered_map<std::type_index, std::unique_ptr<storage_base>> storage_;
};
}  // namespace dmn::detail