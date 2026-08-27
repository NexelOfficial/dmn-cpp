#include "dmn/detail/lmbcs.hpp"

#include <domino/global.h>
#include <domino/dsapi.h>
#include <domino/osmisc.h>

using dmn::lmbcs;

static_assert(sizeof(LMBCS) == sizeof(lmbcs::char_t));

auto lmbcs::cast(lmbcs::view in) noexcept -> const char* {
  return reinterpret_cast<const char*>(in.data());
}

auto lmbcs::cast(const lmbcs::str& in) noexcept -> const char* {
  return reinterpret_cast<const char*>(in.data());
}

auto lmbcs::cast(lmbcs::str& in) noexcept -> char* { return reinterpret_cast<char*>(in.data()); }

auto lmbcs::translate_impl(std::span<const char> inp, std::span<char> out, bool to_utf8) -> size_t {
  const auto mode = to_utf8 ? OS_TRANSLATE_LMBCS_TO_UTF8 : OS_TRANSLATE_UTF8_TO_LMBCS;
  return OSTranslate(mode, inp.data(), inp.size(), out.data(), out.size());
}