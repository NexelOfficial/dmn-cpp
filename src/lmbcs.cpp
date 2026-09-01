#include "dmn/lmbcs.hpp"

#include <domino/global.h>
#include <domino/dsapi.h>
#include <domino/osmisc.h>

using dmn::lmbcs;
using dmn::lmbcs_view;

static_assert(sizeof(LMBCS) == sizeof(dmn::lmbcs::char_t));
static_assert(sizeof(LMBCS) == sizeof(dmn::lmbcs_view::char_t));

namespace {
auto to_string_impl(dmn::lmbcs_view str) -> std::string {
  if (str.empty()) {
    return {};
  }

  std::string out{};
  out.resize(str.size() * 2);

  const auto out_len =
    OSTranslate(OS_TRANSLATE_LMBCS_TO_UTF8, str.data(), str.size(), out.data(), out.size());
  out.resize(out_len);
  return out;
}
}  // namespace

auto lmbcs::from_string(std::string_view str) -> lmbcs {
  if (str.empty()) {
    return {};
  }

  lmbcs out{};
  out.resize(str.size() * 2);

  const auto out_len =
    OSTranslate(OS_TRANSLATE_UTF8_TO_LMBCS, str.data(), str.size(), out.data(), out.size());
  out.resize(out_len);
  return out;
}

auto lmbcs::to_string() const -> std::string { return to_string_impl(*this); }
auto lmbcs::to_string() -> std::string { return to_string_impl(*this); }

auto lmbcs_view::to_string() const -> std::string { return to_string_impl(*this); }
auto lmbcs_view::to_string() -> std::string { return to_string_impl(*this); }