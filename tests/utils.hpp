#pragma once

#include "dmn/database.hpp"

#include <optional>
#include <random>

namespace utils {
struct db_guard {
  struct deleter {
    void operator()(dmn::database* db) const noexcept {
      if (db != nullptr) {
        auto path = db->get_path();
        delete db;
        dmn::database::remove(path);
      }
    }
  };

  std::shared_ptr<dmn::database> value;
  db_guard(dmn::database db) : value(new dmn::database(std::move(db)), deleter{}) {}

  auto operator*() -> dmn::database& { return *value; }
  auto operator->() -> dmn::database* { return &*value; }

  void release() noexcept { value.reset(); }
};

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

inline auto random_database() {
  auto name = random_small_string() + ".nsf";
  auto db = dmn::database::create(name);
  if (!db) {
    throw std::runtime_error("Failed to create random database");
  }
  auto guard = db_guard{std::move(*db)};
  return std::pair{std::move(guard), std::move(name)};
}
}  // namespace utils