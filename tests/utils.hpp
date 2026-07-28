#pragma once

#include <optional>
#include <random>

#include "dmn/nsf/note.hpp"

namespace utils {
struct note_guard {
  std::optional<dmn::note> value;

  note_guard() = default;
  note_guard(dmn::note note) : value(std::move(note)) {}

  note_guard(const note_guard&) = delete;
  auto operator=(const note_guard&) = delete;
  note_guard(note_guard&&) = delete;
  auto operator=(note_guard&&) = delete;

  ~note_guard() {
    if (!value) {
      return;
    }

    try {
      value->remove(true);
    } catch (...) {
    }
    value.reset();
  }

  auto operator*() -> dmn::note& { return *value; }
  auto operator*() const -> const dmn::note& { return *value; }
  auto operator->() -> dmn::note* { return &*value; }
  auto operator->() const -> const dmn::note* { return &*value; }

  void release() noexcept { value.reset(); }
};

inline auto open_database(std::string_view name) -> dmn::database {
  auto db = dmn::database::open(name);
  if (!db) {
    throw std::runtime_error("Failed to open Example.nsf");
  }
  return *db;
}

inline auto random_string(size_t len) -> std::string {
  constexpr static std::string_view RAND_CHARSET = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  static std::random_device rd{};
  static std::mt19937 gen{rd()};

  std::string output;
  for (size_t i = 0; i < len; ++i) {
    std::uniform_int_distribution<> distrib(0, RAND_CHARSET.size() - 1);
    output += RAND_CHARSET.at(distrib(gen));
  }
  return output;
}

inline auto random_small_string() -> std::string {
  constexpr static size_t SMALL_LEN = 32;
  return random_string(SMALL_LEN);
}

inline auto random_large_string() -> std::string {
  constexpr static size_t LARGE_LEN = 0xFFFF;
  return random_string(LARGE_LEN);
}
}  // namespace utils