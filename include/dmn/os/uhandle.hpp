#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <utility>

namespace dmn {
using dhandle_t = uint32_t;

template <typename T>
class uhandle {
 public:
  using cleanup_t = std::function<void(T)>;

  /// Create an empty handle wrapper.
  ///
  /// \param fn Cleanup function that is called when the managed handle is destroyed.
  /// \note The cleanup function is never called when the handle is null.
  explicit uhandle(cleanup_t fn) : cleanup(std::move(fn)) {}

  /// Create a handle wrapper from an existing handle.
  ///
  /// \param handle Handle to manage.
  /// \param fn Cleanup function that is called when the managed handle is destroyed.
  /// \note The cleanup function is never called when the handle is null.
  explicit uhandle(T handle, cleanup_t fn) : hdl_(handle), cleanup(std::move(fn)) {}

  ~uhandle() { reset(); }
  uhandle(const uhandle&) = delete;
  auto operator=(const uhandle&) -> uhandle& = delete;

  uhandle(uhandle&& other) noexcept
      : hdl_(std::exchange(other.hdl_, null_value())), cleanup(std::move(other.cleanup)) {}

  auto operator=(uhandle&& other) noexcept -> uhandle& {
    if (this != &other) {
      reset();
      hdl_ = std::exchange(other.hdl_, null_value());
      cleanup = std::move(other.cleanup);
    }
    return *this;
  }

  constexpr operator bool() noexcept { return hdl_ != null_value(); }
  constexpr operator bool() const noexcept { return hdl_ != null_value(); }

  /// Release the managed handle.
  ///
  /// Sets the handle to a null value preventing the cleanup function from being called.
  /// \return The underlying handle before releasing.
  auto release() noexcept -> T {
    auto old = hdl_;
    hdl_ = null_value();
    return old;
  }

  /// Call the cleanup function and set the handle to a null value.
  ///
  /// \note Does nothing if the cleanup function is empty or the handle is null.
  void reset() noexcept {
    if (cleanup != nullptr && hdl_ != null_value()) {
      cleanup(hdl_);
      hdl_ = null_value();
    }
  }

  /// Replace the managed handle.
  ///
  /// Calls `uhandle::reset()` and starts managing the provided handle.
  void put(T handle) noexcept {
    reset();
    hdl_ = handle;
  }

  /// Get pointer to underlying handle.
  constexpr auto data() noexcept -> T* { return &hdl_; }

  /// Get the raw managed handle.
  ///
  /// \throws std::runtime_error If the managed handle is null.
  [[nodiscard]] auto get() const -> T {
    if (hdl_ == null_value()) {
      throw std::runtime_error("Empty handle accessed");
    }

    return hdl_;
  }

  /// Get the raw managed handle without throwing.
  ///
  /// \return Managed handle, if available.
  [[nodiscard]] auto try_get() const noexcept -> std::optional<T> {
    if (hdl_ == null_value()) {
      return std::nullopt;
    }

    return hdl_;
  }

 private:
  static constexpr auto null_value() noexcept -> T { return T{}; }

  T hdl_ = null_value();
  cleanup_t cleanup;
};
}  // namespace dmn