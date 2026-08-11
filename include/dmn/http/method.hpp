#pragma once

#include <cstdint>
#include <string_view>

namespace dmn::http {
enum class method : uint8_t {
  none,
  head,
  get,
  post,
  put,
  del,
  trace,
  connect,
  options,
  unknown,
  bad
};

static auto get_method_as_string(method m) noexcept -> std::string_view {
  switch (m) {
    case method::head:
      return "HEAD";
    case method::get:
      return "GET";
    case method::post:
      return "POST";
    case method::put:
      return "PUT";
    case method::del:
      return "DELETE";
    case method::trace:
      return "TRACE";
    case method::connect:
      return "CONNECT";
    case method::options:
      return "OPTIONS";
    default:
      return "BAD";
  }
}
}  // namespace dmn::http
