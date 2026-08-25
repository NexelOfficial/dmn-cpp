#include "dmn/formula.hpp"

#include <domino/global.h>
#include <domino/nsfsearc.h>
#include <domino/osmem.h>

#include "dmn/detail/lmbcs.hpp"
#include "dmn/detail/locker.hpp"
#include "dmn/error.hpp"

using dmn::formula;

formula::formula() : hdl_(OSMemFree) {};

formula::formula(std::span<uint8_t> buffer) : hdl_(OSMemFree) {
  auto obj = detail::locker::allocate(buffer);
  hdl_.put(obj->release().block);
}

auto formula::compile(std::string_view command, std::string name) -> formula {
  formula data{};
  uint16_t skip = 0;

  const dmn::status result = NSFFormulaCompile(
    name.empty() ? nullptr : name.data(), name.size(), command.data(), command.size(),
    data.hdl_.data(), &skip, &skip, &skip, &skip, &skip, &skip
  );
  result.throw_if_error("Failed to compile formula");

  return data;
}

auto formula::decompile(bool is_selection_formula) const -> std::string {
  dmn::detail::dhandle_t text_hdl{};
  uint16_t text_len = 0;

  const dmn::detail::locker input(get_handle(), size());
  const dmn::status result = NSFFormulaDecompile(
    reinterpret_cast<char*>(input.get_pointer()), is_selection_formula ? TRUE : FALSE, &text_hdl,
    &text_len
  );
  result.throw_if_error("Failed to decompile formula");

  const dmn::detail::locker output(text_hdl, text_len);
  const lmbcs::view raw(output.get_pointer(), text_len);
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
  const lmbcs::str converted = lmbcs::translate(item_name);
  const dmn::status result =
    NSFFormulaSummaryItem(get_handle(), lmbcs::cast(converted), converted.size());
  result.throw_if_error("Failed to add summary item to formula");
}
