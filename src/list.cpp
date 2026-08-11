#include "dmn/list.hpp"

#include <domino/global.h>
#include <domino/osmem.h>
#include <domino/textlist.h>
#include <domino/nsfnote.h>

#include <limits>
#include <stdexcept>

#include "dmn/detail/lmbcs.hpp"
#include "dmn/detail/locker.hpp"
#include "dmn/error.hpp"

using dmn::list;

constexpr uint16_t MAX_UINT16 = std::numeric_limits<uint16_t>::max();

list::list() : hdl_(OSMemFree) {
  const dmn::status result = ListAllocate(0, 0, TRUE, hdl_.data(), nullptr, &size_);
  result.throw_if_error("Failed to allocate list");
  OSUnlock(hdl_.get());
}

list::list(void* existing) : hdl_(OSMemFree) {
  const uint16_t type = *static_cast<uint16_t*>(existing);
  if (type != TYPE_TEXT_LIST) {
    throw std::invalid_argument("Provided pointer is not of a text list");
  }

  const dmn::status result = ListDuplicate(static_cast<LIST*>(existing), TRUE, hdl_.data());
  result.throw_if_error("Failed to duplicate list");
  OSUnlock(hdl_.get());
}

auto list::empty() const -> bool { return size() == 0; }

auto list::size() const -> size_t {
  auto list = detail::locker(hdl_.get(), size_, detail::ownership::borrow);
  return ListGetNumEntries(list.get_pointer(), TRUE);
}

auto list::buffer_size() const -> uint16_t {
  auto list = detail::locker(hdl_.get(), size_, detail::ownership::borrow);
  return ListGetSize(list.get_pointer(), TRUE);
}

auto list::at(size_t index) const -> std::string {
  if (index >= size()) {
    throw std::out_of_range("List index is out of range");
  }

  char* text = nullptr;
  uint16_t text_size = 0;
  auto list = detail::locker(hdl_.get(), size_, detail::ownership::borrow);
  const dmn::status result = ListGetText(list.get_pointer(), TRUE, index, &text, &text_size);
  result.throw_if_error("Failed to get list entry");

  const auto* data = reinterpret_cast<const uint8_t*>(text);
  return dmn::lmbcs::translate(dmn::lmbcs::view(data, text_size));
}

void list::push_back(const std::string& value) {
  const dmn::lmbcs::str converted = dmn::lmbcs::translate(value);
  const size_t value_len = converted.size();
  if (value_len > MAX_UINT16) {
    throw std::out_of_range("Text list entry too large");
  }
  const size_t count = size();
  if (count > MAX_UINT16) {
    throw std::out_of_range("Text list has too many entries");
  }

  const dmn::status result =
    ListAddEntry(hdl_.get(), TRUE, &size_, count, dmn::lmbcs::cast(converted), value_len);
  result.throw_if_error("Failed to add list entry");
}

void list::pop_back() {
  const size_t count = size();
  if (count == 0) {
    throw std::out_of_range("List is empty");
  }

  erase(count - 1);
}

void list::erase(size_t index) {
  if (index >= size()) {
    throw std::out_of_range("List index is out of range");
  }

  const dmn::status result = ListRemoveEntry(hdl_.get(), TRUE, &size_, index);
  result.throw_if_error("Failed to remove list entry");
}

void list::clear() {
  const dmn::status result = ListRemoveAllEntries(hdl_.get(), TRUE, &size_);
  result.throw_if_error("Failed to clear list");
}

auto list::begin() const -> const_iterator { return const_iterator{this, 0}; }
auto list::end() const -> const_iterator { return const_iterator{this, size()}; }
auto list::cbegin() const -> const_iterator { return begin(); }
auto list::cend() const -> const_iterator { return end(); }