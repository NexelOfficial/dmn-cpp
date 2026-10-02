#include <catch2/catch_test_macros.hpp>

#include "dmn/detail/locker.hpp"
#include "dmn/detail/hook.hpp"

namespace detail = dmn::detail;

constexpr uint8_t CHUNK_SIZE = 0xFF;
constexpr uint8_t ALLOC_ITERATIONS = 0xF;

TEST_CASE("Reused handles stay invalid", "[handle]") {
  detail::block_id bid;
  {
    const auto locker = detail::locker::allocate(CHUNK_SIZE);
    REQUIRE(detail::hook::is_valid(locker.get_handle()));
    REQUIRE(locker.get_block_id().valid());

    bid = locker.get_block_id();
  }

  REQUIRE_FALSE(bid.valid());

  for (size_t i = 0; i < ALLOC_ITERATIONS; i++) {
    const auto locker = detail::locker::allocate(CHUNK_SIZE);
    REQUIRE(detail::hook::is_valid(locker.get_handle()));
    REQUIRE(locker.get_block_id().valid());
    REQUIRE_FALSE(bid.valid());
  }
}