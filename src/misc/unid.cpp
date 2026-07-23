#include "dmn/misc/unid.hpp"

#include <charconv>
#include <format>
#include <ranges>

#include <domino/global.h>
#include <domino/nsfdata.h>
#include <domino/nsfnote.h>
#include <domino/fsods.h>

using dmn::unid;

static_assert(sizeof(UNID) == sizeof(dmn::unid));
static_assert(alignof(UNID) == alignof(dmn::unid));

static_assert(sizeof(OID) == sizeof(dmn::oid));
static_assert(alignof(OID) == alignof(dmn::oid));

static_assert(sizeof(NOTEID) == sizeof(dmn::note_id::value));

auto unid::from_string(std::string_view str) -> std::optional<unid> {
  if (str.size() != MAXUNIDSTRING) {
    return std::nullopt;
  }

  std::array<uint32_t, 4> parts{};

  for (const auto [i, part] : parts | std::views::enumerate) {
    const auto chunk = str.substr(i * 8, 8);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const auto [ptr, ec] = std::from_chars(chunk.data(), chunk.data() + chunk.size(), part, 16);
    if (ec != std::errc{}) {
      return std::nullopt;
    }
  }

  return unid{
    .file = {parts.at(1), parts.at(0)},
    .note = {parts.at(3), parts.at(2)},
  };
}

auto unid::to_string() const -> std::string {
  return std::format("{:08X}{:08X}{:08X}{:08X}", file.at(1), file.at(0), note.at(1), note.at(0));
}

auto unid::as_raw_unid() noexcept -> UNIVERSALNOTEID_tag* {
  return reinterpret_cast<UNIVERSALNOTEID_tag*>(this);
}