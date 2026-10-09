#include "dmn/detail/random.hpp"

#include <random>

namespace detail = dmn::detail;

auto detail::random_string(size_t size) -> std::string {
  constexpr static std::string_view CHARACTER_SET = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  static std::random_device rd{};
  static std::mt19937 gen{rd()};

  std::string result;
  result.reserve(size);

  for (size_t i = 0; i < size; ++i) {
    std::uniform_int_distribution<> distrib(0, CHARACTER_SET.size() - 1);
    result.push_back(CHARACTER_SET.at(distrib(gen)));
  }
  return result;
};