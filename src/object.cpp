#include "dmn/object.hpp"

#include <domino/global.h>
#include <domino/nsfnote.h>
#include <domino/misc.h>

#include <cstring>

#include "dmn/detail/locker.hpp"
#include "dmn/time_date.hpp"
#include "dmn/error.hpp"

using dmn::object;

object::object(detail::locker locker) : bid_(locker.get_block_id()), size_(locker.size()) {
  if (locker.get_ownership() != detail::ownership::take) {
    throw dmn::invalid_argument("Locker must own memory in order to create an object");
  }

  owner_ = std::make_shared<detail::locker>(std::move(locker));
}

auto object::empty() const noexcept -> bool { return size_ <= 2; }

auto object::get_type() const -> dmn::type {
  if (size_ < 2 || *bid_ == detail::block_id{}) {
    return dmn::type::invalid_or_unknown;
  }

  auto obj = get_cursor();
  return obj.read<dmn::type>();
}

auto object::as_string() const -> std::optional<std::string> {
  if (size_ < 2 || *bid_ == detail::block_id{}) {
    return std::nullopt;
  }

  auto [typ, obj] = data_pair();
  const size_t data_size = size_ - sizeof(typ);

  if (typ == dmn::type::text) {
    // Use pointer with dmn::lmbcs_view instead of obj.read() to prevent double allocation
    const dmn::lmbcs_view value(obj.get_pointer<dmn::lmbcs::char_t>(), data_size);
    return value.to_string();
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
      const dmn::lmbcs_view out(obj.get_pointer<dmn::lmbcs::char_t>(), len);
      output += out.to_string() + ";";
      obj.advance_offset(len);
    }

    return output.empty() ? "" : output.substr(0, output.size() - 1);
  }

  return std::nullopt;
}

auto object::get_cursor() const -> detail::locker {
  return {bid_.get(), size_, detail::ownership::borrow};
}

auto object::data_pair() const -> std::pair<dmn::type, detail::locker> {
  auto obj = get_cursor();
  const auto typ = obj.read<dmn::type>();
  return {typ, std::move(obj)};
}