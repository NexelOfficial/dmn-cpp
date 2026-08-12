#include "dmn/note.hpp"

#include <domino/global.h>
#include <domino/nsfnote.h>
#include <domino/osmem.h>

#include "dmn/error.hpp"

using dmn::note;

namespace {
auto get_flags(size_t size) -> uint16_t {
  const bool is_summary = size < MAXONESEGSIZE / 4;
  return is_summary ? ITEM_SUMMARY : 0;
};
}  // namespace

auto note::get_impl(const lmbcs::str& key) const -> std::optional<dmn::object> {
  detail::block_id item_bid{};
  uint16_t item_type = 0;
  detail::block_id value_bid{};
  DWORD value_len = 0;

  const dmn::status result = NSFItemInfo(
    get_handle(), lmbcs::cast(key), key.size(), reinterpret_cast<BLOCKID*>(&item_bid), &item_type,
    reinterpret_cast<BLOCKID*>(&value_bid), &value_len
  );

  if (result.is_not_found()) {
    return std::nullopt;
  }
  result.throw_if_error("Failed to get item on note");

  auto owner = std::make_shared<dmn::note>(*this);
  return dmn::object{value_bid, value_len, owner, item_bid};
}

void note::append_impl(
  std::string_view key, dmn::type type, const void* data, uint16_t size
) const {
  const lmbcs::str converted = lmbcs::translate(key);
  const auto data_type = static_cast<uint16_t>(type);

  const dmn::status result = NSFItemAppend(
    get_handle(), get_flags(size), lmbcs::cast(converted), converted.size(), data_type, data, size
  );
  result.throw_if_error("Failed to append item value");
}

void note::modify_impl(
  std::string_view key, dmn::type type, const void* data, uint16_t size
) const {
  auto existing = get<dmn::object>(key);
  if (!existing) {
    throw dmn::invalid_argument("Provided key doesn't exist on note");
  }

  const std::span<const uint8_t> buffer{reinterpret_cast<const uint8_t*>(data), size};
  existing->write(type, buffer);
}