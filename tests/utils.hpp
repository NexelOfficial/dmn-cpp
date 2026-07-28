#pragma once

#include <optional>

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
}  // namespace utils