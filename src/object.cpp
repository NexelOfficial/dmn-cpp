#include "dmn/object.hpp"

#include <domino/global.h>
#include <domino/nsfnote.h>
#include <domino/misc.h>

#include <cstring>

#include "dmn/detail/locker.hpp"
#include "dmn/time_date.hpp"
#include "dmn/error.hpp"

using dmn::object;

auto object::empty() const noexcept -> bool { return !state_ || state_->size <= 2; }

auto object::get_type() const -> dmn::type {
  if (!state_ || state_->size < 2 || state_->bid.pool == detail::dhandle_t{}) {
    return dmn::type::invalid_or_unknown;
  }

  detail::locker obj(state_->bid, state_->size, detail::ownership::borrow);
  return obj.read<dmn::type>();
}

void object::write(dmn::type typ, std::span<const std::byte> data) {
  if (!item_bid_) {
    throw dmn::runtime_error("Object that is not an item value can't be overwritten");
  }

  auto st = ensure_state();
  const size_t new_size = data.size() + sizeof(dmn::type);
  if (new_size != st.size) {
    const auto raw_item_bid = *reinterpret_cast<BLOCKID*>(&*item_bid_);
    const dmn::status result =
      NSFItemRealloc(raw_item_bid, reinterpret_cast<BLOCKID*>(&st.bid), new_size);
    result.throw_if_error("Failed to reallocate object memory");
  }

  st.size = new_size;
  detail::locker pool(st.bid, st.size, detail::ownership::borrow);
  pool.write(typ);
  pool.write(data);
}

auto object::as_string() const -> std::optional<std::string> {
  if (!state_ || state_->size < 2 || state_->bid.pool == detail::dhandle_t{}) {
    return std::nullopt;
  }

  auto [typ, obj] = data_pair();
  const size_t data_size = state_->size - sizeof(typ);

  if (typ == dmn::type::text) {
    // Use pointer with lmbcs::view instead of obj.read() to prevent double allocation
    const lmbcs::view value(obj.get_pointer<lmbcs::char_t>(), data_size);
    return lmbcs::translate(value);
  }
  if (typ == dmn::type::number && data_size == sizeof(double)) {
    constexpr static uint8_t MAX_DOUBLE_SIZE = 32;
    std::array<char, MAX_DOUBLE_SIZE> buffer{};
    const auto num = obj.read<double>();

    const auto [ptr, ec] =
      std::to_chars(buffer.data(), std::to_address(buffer.end()), num, std::chars_format::general);
    if (ec != std::errc{}) {
      return std::nullopt;
    }

    return std::string(buffer.data(), ptr);
  }
  if (typ == dmn::type::time && data_size == sizeof(dmn::time_date)) {
    const auto td = obj.read<TIMEDATE>();
    std::string output(MAXALPHATIMEDATE + 1, '\0');
    auto res = ConvertTIMEDATEtoRFC3339Date(&td, output.data(), MAXALPHATIMEDATE);
    if (res != NOERROR) {
      return std::nullopt;
    }
    output.resize(output.find('\0'));
    return output;
  }
  if (typ == dmn::type::text_list && data_size >= sizeof(uint16_t)) {
    const auto entries = obj.read<uint16_t>();
    std::vector<uint16_t> lengths{};
    lengths.reserve(entries);

    for (uint16_t i = 0; i < entries; i++) {
      lengths.emplace_back(obj.read<uint16_t>());
    }

    std::string output;
    for (const auto& len : lengths) {
      const lmbcs::view out(obj.get_pointer<lmbcs::char_t>(), len);
      output += lmbcs::translate(out) + ";";
      obj.advance_offset(len);
    }

    return output.empty() ? "" : output.substr(0, output.size() - 1);
  }

  return std::nullopt;
}

auto object::get_cursor() const -> detail::locker {
  return {state_->bid, state_->size, detail::ownership::borrow};
}

auto object::ensure_state() -> state& {
  if (!state_) {
    throw dmn::runtime_error("Object does not have a valid state.");
  }
  return *state_;
}

auto object::ensure_state() const -> const state& {
  if (!state_) {
    throw dmn::runtime_error("Object does not have a valid state.");
  }
  return *state_;
}

auto object::data_pair() const -> std::pair<dmn::type, detail::locker> {
  detail::locker obj(state_->bid, state_->size, detail::ownership::borrow);
  const auto typ = obj.read<dmn::type>();
  return {typ, std::move(obj)};
}