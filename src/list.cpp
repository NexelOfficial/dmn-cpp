#include "dmn/list.hpp"

#include <domino/global.h>
#include <domino/osmem.h>
#include <domino/textlist.h>
#include <domino/nsfnote.h>

#include <limits>

#include "dmn/detail/locker.hpp"
#include "dmn/detail/scoped_handle.hpp"
#include "dmn/lmbcs.hpp"
#include "dmn/error.hpp"

using dmn::list;

constexpr uint16_t MAX_UINT16 = std::numeric_limits<uint16_t>::max();

list::list() : hdl_(OSMemFree) {
  detail::dhandle_t handle{};
  const dmn::status result = ListAllocate(0, 0, TRUE, &handle, nullptr, &size_);
  result.throw_if_error("Failed to allocate list");

  hdl_.put(handle);
  if (handle != detail::dhandle_t{}) {
    OSUnlock(handle);
  }
}

list::list(std::span<std::byte> buffer) : hdl_(OSMemFree) {
  const uint16_t type = *reinterpret_cast<uint16_t*>(buffer.data());
  if (type != TYPE_TEXT_LIST) {
    throw dmn::invalid_argument("Provided pointer is not a text list");
  }

  detail::dhandle_t handle{};
  const dmn::status result = ListDuplicate(reinterpret_cast<LIST*>(buffer.data()), TRUE, &handle);
  result.throw_if_error("Failed to duplicate list");

  hdl_.put(handle);
  if (handle != detail::dhandle_t{}) {
    OSUnlock(handle);
  }
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
    throw dmn::out_of_range("List index is out of range");
  }

  char* text = nullptr;
  uint16_t text_size = 0;
  auto list = detail::locker(hdl_.get(), size_, detail::ownership::borrow);
  const dmn::status result = ListGetText(list.get_pointer(), TRUE, index, &text, &text_size);
  result.throw_if_error("Failed to get list entry");

  const auto* data = reinterpret_cast<dmn::lmbcs::char_t*>(text);
  return dmn::lmbcs_view(data, text_size).to_string();
}

void list::push_back(std::string_view value) {
  const auto converted = dmn::lmbcs::from_string(value);
  const size_t value_len = converted.size();
  if (value_len > MAX_UINT16) {
    throw dmn::out_of_range("Text list entry too large");
  }
  const size_t count = size();
  if (count > MAX_UINT16) {
    throw dmn::out_of_range("Text list has too many entries");
  }

  const dmn::status result =
    ListAddEntry(hdl_.get(), TRUE, &size_, count, converted.c_str(), value_len);
  result.throw_if_error("Failed to add list entry");
}

void list::pop_back() {
  const size_t count = size();
  if (count == 0) {
    throw dmn::out_of_range("List is empty");
  }

  erase(count - 1);
}

void list::erase(size_t index) {
  if (index >= size()) {
    throw dmn::out_of_range("List index is out of range");
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