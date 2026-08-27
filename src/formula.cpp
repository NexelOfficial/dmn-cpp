#include "dmn/formula.hpp"

#include <domino/global.h>
#include <domino/nsfsearc.h>
#include <domino/osmem.h>

#include "dmn/detail/lmbcs.hpp"
#include "dmn/detail/locker.hpp"
#include "dmn/error.hpp"

using dmn::formula;

formula::formula(handle_t hdl) : hdl_(hdl, OSMemFree) {};

formula::formula(std::span<std::byte> buffer) : hdl_(OSMemFree) {
  auto obj = detail::locker::allocate<std::byte>(buffer);
  hdl_.put(obj.release().block);
}

formula::formula(std::string_view command) : hdl_(OSMemFree) {
  uint16_t skip = 0;
  dmn::status compile_error = dmn::no_error;
  const auto converted = lmbcs::translate(command);
  const dmn::status result = NSFFormulaCompile(
    nullptr, 0, lmbcs::cast(converted), converted.size(), hdl_.data(), &skip, &compile_error.value,
    &skip, &skip, &skip, &skip
  );
  compile_error.throw_if_error("Compile error");
  result.throw_if_error("Failed to compile formula");
}

auto formula::decompile(bool is_selection_formula) const -> std::string {
  detail::dhandle_t text_hdl{};
  uint16_t text_len = 0;

  const detail::locker input(get_handle(), size());
  const dmn::status result = NSFFormulaDecompile(
    input.get_pointer<char>(), is_selection_formula ? TRUE : FALSE, &text_hdl, &text_len
  );
  result.throw_if_error("Failed to decompile formula");

  const detail::locker output(text_hdl, text_len);
  const lmbcs::view raw(output.get_pointer<lmbcs::char_t>(), text_len);
  return lmbcs::translate(raw);
}

auto formula::size() const -> size_t {
  uint16_t size = 0;
  NSFFormulaGetSize(get_handle(), &size);
  return size;
}

void formula::merge(const formula& other) const {
  const dmn::status result = NSFFormulaMerge(other.get_handle(), get_handle());
  result.throw_if_error("Failed to merge formulas");
}

void formula::add_summary(std::string_view item_name) const {
  add_summary(lmbcs::translate(item_name));
}

void formula::add_summary(lmbcs::view item_name) const {
  const dmn::status result =
    NSFFormulaSummaryItem(get_handle(), lmbcs::cast(item_name), item_name.size());
  result.throw_if_error("Failed to add summary item to formula");
}

void formula::add_item_name(std::string_view item_name) const {
  add_item_name(lmbcs::translate(item_name));
}

void formula::add_item_name(lmbcs::view item_name) const {
  header hdr{};
  {
    auto cursor = get_cursor();
    hdr = cursor.read<header>();
  }

  if (hdr.flags != 0) {
    throw dmn::runtime_error("Formula already has an item name");
  }

  const auto section_size = sizeof(uint16_t) + item_name.size();
  const auto padding = section_size & 1U;
  const auto padded_size = section_size + padding;
  const auto new_length = hdr.length + padded_size;

  const dmn::status result = OSMemRealloc(get_handle(), new_length);
  result.throw_if_error("Failed to resize formula memory");

  auto cursor = get_cursor();

  std::memmove(
    cursor.get_pointer(sizeof(header) + padded_size), cursor.get_pointer(sizeof(header)),
    hdr.length - sizeof(header)
  );

  const header new_hdr{
    .length = static_cast<uint16_t>(new_length),
    .flags = 2,
    .offset = static_cast<uint16_t>(hdr.offset + padded_size)
  };

  cursor.set_offset(0);
  cursor.write(new_hdr);
  cursor.write(static_cast<uint16_t>(item_name.size()));
  cursor.write(std::as_bytes(std::span{item_name.data(), item_name.size()}));

  if (padding != 0) {
    cursor.write(std::byte{0});
  }
}