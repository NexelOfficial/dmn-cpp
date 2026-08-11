#include "dmn/object.hpp"

#include <domino/global.h>
#include <domino/nsfnote.h>
#include <domino/misc.h>

#include <cstring>
#include <stdexcept>
#include <vector>

#include "dmn/detail/locker.hpp"
#include "dmn/time_date.hpp"
#include "dmn/error.hpp"

using dmn::object;

void object::write(dmn::type typ, std::span<const uint8_t> data) {
  if (!item_bid_) {
    throw std::runtime_error("Object that is not an item value can't be overwritten");
  }

  const size_t new_size = data.size() + sizeof(dmn::type);
  if (new_size != size_) {
    const auto raw_item_bid = *reinterpret_cast<BLOCKID*>(&*item_bid_);
    const dmn::status result =
      NSFItemRealloc(raw_item_bid, reinterpret_cast<BLOCKID*>(&bid_), new_size);
    result.throw_if_error("Failed to reallocate object memory");
  }

  size_ = new_size;
  detail::locker pool(bid_, size_, detail::ownership::borrow);
  pool.write(&typ);
  pool.write(data.data(), data.size());
}

auto object::as_string() const -> std::optional<std::string> {
  if (size_ < 2 || bid_.pool == detail::dhandle_t{}) {
    return std::nullopt;
  }

  detail::locker obj(bid_, size_, detail::ownership::borrow);
  auto typ = obj.read<dmn::type>();
  const size_t data_size = size_ - sizeof(typ);

  if (typ == dmn::type::text) {
    // Use pointer with lmbcs::view instead of obj.read() to prevent double allocation
    auto* ptr = obj.get_pointer();
    const lmbcs::view value(ptr, data_size);
    return lmbcs::translate(value);
  }
  if (typ == dmn::type::number && data_size == sizeof(double)) {
    constexpr static uint8_t MAX_DOUBLE_SIZE = 32;
    std::array<char, MAX_DOUBLE_SIZE> buffer{};

    const auto [ptr, ec] = std::to_chars(
      buffer.data(), std::to_address(buffer.end()), obj.read<double>(), std::chars_format::general
    );
    if (ec != std::errc{}) {
      return std::nullopt;
    }

    return std::string(buffer.data(), ptr);
  }
  if (typ == dmn::type::time && data_size == sizeof(dmn::time_date)) {
    auto td = obj.read<TIMEDATE>();
    std::string output(MAXALPHATIMEDATE + 1, '\0');
    uint16_t output_len = 0;
    auto res =
      ConvertTIMEDATEToText(nullptr, nullptr, &td, output.data(), MAXALPHATIMEDATE, &output_len);
    if (res != NOERROR) {
      return std::nullopt;
    }
    output.resize(output_len);
    return output;
  }
  if (typ == dmn::type::text_list && data_size >= sizeof(uint16_t)) {
    auto entries = obj.read<uint16_t>();
    std::vector<uint16_t> lengths{};
    lengths.reserve(entries);

    for (uint16_t i = 0; i < entries; i++) {
      lengths.emplace_back(obj.read<uint16_t>());
    }

    std::string output;
    for (const auto& len : lengths) {
      const lmbcs::view out(obj.get_pointer(), len);
      output += lmbcs::translate(out) + ";";
      obj.increment_offset(len);
    }

    return output.empty() ? "" : output.substr(0, output.size() - 1);
  }

  return std::nullopt;
}