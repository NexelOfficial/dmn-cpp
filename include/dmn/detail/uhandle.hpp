#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <cassert>
#include <thread>
#include <utility>

#include "dmn/error.hpp"

namespace dmn::detail {
using dhandle_t = uint32_t;

template <typename T>
/// Wrapper for raw handles to properly manage them.
///
/// \throws dmn::thread_error If accessed from a different thread.
class uhandle {
 public:
  using cleanup_t = std::function<void(T)>;

  /// Create an empty handle wrapper.
  ///
  /// \param fn Cleanup function that is called when the managed handle is destroyed.
  /// \note The cleanup function is never called when the handle is null.
  explicit uhandle(cleanup_t fn) noexcept
      : cleanup(std::move(fn)), owner_(std::this_thread::get_id()) {}

  /// Create a handle wrapper from an existing handle.
  ///
  /// \param handle Handle to manage.
  /// \param fn Cleanup function that is called when the managed handle is destroyed.
  /// \note The cleanup function is never called when the handle is null.
  explicit uhandle(T handle, cleanup_t fn) noexcept
      : hdl_(handle), cleanup(std::move(fn)), owner_(std::this_thread::get_id()) {}

  ~uhandle() noexcept {
    assert(owner_ == std::this_thread::get_id());
    if (!is_owner_thread()) {
      std::terminate();
    }

    reset_unchecked();
  }

  uhandle(const uhandle&) = delete;
  auto operator=(const uhandle&) -> uhandle& = delete;

  uhandle(uhandle&& other) noexcept
      : hdl_(std::exchange(other.hdl_, null_value())),
        cleanup(std::move(other.cleanup)),
        owner_(other.owner_) {}

  auto operator=(uhandle&& other) noexcept -> uhandle& {
    if (this != &other) {
      assert(owner_ == std::this_thread::get_id());
      if (!is_owner_thread()) {
        std::terminate();
      }

      reset_unchecked();
      hdl_ = std::exchange(other.hdl_, null_value());
      cleanup = std::move(other.cleanup);
      owner_ = other.owner_;
    }

    return *this;
  }

  constexpr operator bool() const noexcept { return hdl_ != null_value(); }

  /// Release the managed handle.
  ///
  /// Sets the handle to a null value preventing the cleanup function from being called.
  /// \return The underlying handle before releasing.
  auto release() -> T {
    check_thread();
    auto old = hdl_;
    hdl_ = null_value();
    return old;
  }

  /// Call the cleanup function and set the handle to a null value.
  ///
  /// \note Does nothing if the cleanup function is empty or the handle is null.
  void reset() {
    check_thread();
    reset_unchecked();
  }

  /// Replace the managed handle.
  ///
  /// Calls `uhandle::reset()` and starts managing the provided handle.
  void put(T handle) {
    check_thread();
    reset();
    hdl_ = handle;
  }

  /// Get pointer to underlying handle.
  auto data() -> T* {
    check_thread();
    return &hdl_;
  }

  /// Get the raw managed handle.
  ///
  /// \throws dmn::invalid_handle If the managed handle is null.
  [[nodiscard]] auto get() const -> T {
    check_thread();
    if (hdl_ == null_value()) {
      throw dmn::invalid_handle("Empty handle accessed");
    }

    return hdl_;
  }

  /// Get the raw managed handle without throwing.
  ///
  /// \return Managed handle, if available.
  [[nodiscard]] auto try_get() const noexcept -> std::optional<T> {
    if (hdl_ == null_value() || !is_owner_thread()) {
      return std::nullopt;
    }

    return hdl_;
  }

  [[nodiscard]] auto is_owner_thread() const noexcept -> bool {
    return owner_ == std::this_thread::get_id();
  }

 private:
  static constexpr auto null_value() noexcept -> T { return T{}; }

  void check_thread() const {
    if (!is_owner_thread()) {
      throw dmn::thread_error("Handle accessed from non-owning thread");
    }
  }

  void reset_unchecked() noexcept {
    if (cleanup != nullptr && hdl_ != null_value()) {
      cleanup(hdl_);
      hdl_ = null_value();
    }
  }

  T hdl_ = null_value();
  cleanup_t cleanup;
  std::thread::id owner_;
};
}  // namespace dmn::detail