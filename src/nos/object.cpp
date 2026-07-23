#include "dmn/nos/object.hpp"

using dmn::object;

auto object::as_string() const -> std::optional<std::string> {
  if (size_ < 2 || bid_.pool == dmn::dhandle_t{}) {
    return std::nullopt;
  }

  dmn::os::locker obj(bid_, size_, dmn::os::ownership::borrow);
  auto typ = obj.read<dmn::type>();
  const size_t data_size = size_ - sizeof(typ);

  if (typ == dmn::type::text) {
    // Use pointer with lmbcs::view instead of obj.read() to prevent double allocation
    auto* ptr = obj.get_pointer();
    const lmbcs::view value(ptr, data_size);
    return lmbcs::translate(value);
  }
  if (typ == dmn::type::number && data_size == sizeof(double)) {
    return std::to_string(obj.read<double>());
  }
  if (typ == dmn::type::text_list && data_size >= sizeof(uint16_t)) {
    auto entries = obj.read<uint16_t>();
    std::vector<uint16_t> lengths(entries);
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