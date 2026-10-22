#include <catch2/catch_test_macros.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <functional>
#include <string>

#include "dmn/http/method.hpp"
#include "dmn/lmbcs.hpp"
#include "dmn/thread.hpp"
#include "dmn/time_date.hpp"
#include "dmn/unid.hpp"

TEST_CASE("LMBCS strings convert UTF-8 text", "[core][lmbcs]") {
  const std::string text = "Hello, 🐶!";
  auto encoded = dmn::lmbcs::from_string(text);

  REQUIRE_FALSE(encoded.empty());
  REQUIRE(encoded.to_string() == text);
  REQUIRE(dmn::lmbcs_view{encoded}.to_string() == text);
  REQUIRE(dmn::lmbcs::from_string("").to_string().empty());
}

TEST_CASE("UNIDs parse, format, expose native storage, and hash", "[core][unid]") {
  constexpr std::string_view text = "0123456789ABCDEFFEDCBA9876543210";
  auto value = dmn::unid::from_string(text);

  REQUIRE(value.has_value());
  REQUIRE(value->to_string() == text);
  REQUIRE(value->as_raw_unid() != nullptr);
  REQUIRE(std::hash<dmn::unid>{}(*value) == std::hash<dmn::unid>{}(*value));
  REQUIRE_FALSE(dmn::unid::from_string("too short").has_value());
  REQUIRE_FALSE(dmn::unid::from_string("0123456789ABCDEFFEDCBA987654321Z too large").has_value());

  dmn::note_id noteid{42};
  REQUIRE(noteid.data() == &noteid.value);
  REQUIRE(std::hash<dmn::note_id>{}(noteid) == std::hash<dmn::note_id::value_t>{}(42));
}

TEST_CASE("time dates convert whole-second time points", "[core][time]") {
  using namespace std::chrono;
  const auto input = system_clock::time_point{seconds{1769945696}};
  auto value = dmn::time_date::from_time_point(input);

  REQUIRE(value.as_raw_time_date() != nullptr);
  REQUIRE(value.to_time_point().has_value());
  REQUIRE(value.to_time_point().value() == input);
}

TEST_CASE("HTTP methods have stable names", "[core][http]") {
  using enum dmn::http::method;
  const std::array cases{
    std::pair{head, "HEAD"},       std::pair{get, "GET"},         std::pair{post, "POST"},
    std::pair{put, "PUT"},         std::pair{del, "DELETE"},      std::pair{trace, "TRACE"},
    std::pair{connect, "CONNECT"}, std::pair{options, "OPTIONS"}, std::pair{none, "BAD"},
    std::pair{unknown, "BAD"},     std::pair{bad, "BAD"},
  };

  for (const auto& [method, expected] : cases) {
    REQUIRE(dmn::http::get_method_as_string(method) == expected);
  }
}

TEST_CASE("Domino-aware threads expose lifecycle and stop operations", "[core][thread]") {
  std::atomic_bool ran = false;
  dmn::thread worker([&](const std::stop_token& token) {
    ran = true;
    while (!token.stop_requested()) {
      std::this_thread::yield();
    }
  });

  REQUIRE(worker.joinable());
  REQUIRE(worker.get_id() != std::jthread::id{});
  worker.request_stop();
  worker.join();
  REQUIRE_FALSE(worker.joinable());
  REQUIRE(ran.load());
}

TEST_CASE("username is a valid string", "[kfm][session]") {
  const auto username = dmn::detail::runtime::get_username();
  REQUIRE_FALSE(username.empty());
  REQUIRE(username.starts_with("CN="));
  REQUIRE(username.at(username.size() - 1) != '\0');
}