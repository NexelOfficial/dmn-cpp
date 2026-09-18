#include "dmn/detail/attached_object.hpp"

#include <domino/global.h>
#include <domino/nsfnote.h>
#include <domino/nsfobjec.h>

#include "dmn/error.hpp"

using dmn::detail::attached_object;
using type = dmn::detail::attached_object::type;

static_assert(sizeof(attached_object::handle_t) == sizeof(DWORD));

static_assert(static_cast<uint16_t>(type::file) == OBJECT_FILE);
static_assert(static_cast<uint16_t>(type::assist_run_data) == OBJECT_ASSIST_RUNDATA);
static_assert(static_cast<uint16_t>(type::filter_left_to_do) == OBJECT_FILTER_LEFTTODO);
static_assert(static_cast<uint16_t>(type::unknown) == OBJECT_UNKNOWN);

attached_object::attached_object(dmn::database db, type object_type, size_t size)
    : db_(std::move(db)),
      object_type_(object_type),
      hdl_([this](handle_t hdl) { NSFDbFreeObject(db_.get_handle(), hdl); }) {
  handle_t handle = {};
  const dmn::status result =
    NSFDbAllocObject(db_.get_handle(), size, NOTE_CLASS_DOCUMENT, 0, &handle);
  result.throw_if_error("Failed to allocate attached object memory");

  hdl_.put(handle);
}

void attached_object::append_to_note(const dmn::note& note, std::string_view key) const {
  const auto item_size = sizeof(dmn::type) + ods::size(ods::type::object_descriptor);

  const OBJECT_DESCRIPTOR desc{
    .ObjectType = static_cast<uint16_t>(object_type_), .RRV = hdl_.get()
  };

  auto lock = detail::locker::allocate(item_size);
  lock.write(dmn::type::object);
  lock.write(desc, ods::type::object_descriptor);

  const auto converted = dmn::lmbcs::from_string(key);
  const BLOCKID bid{.pool = lock.get_handle(), .block = 0};
  const dmn::status result = NSFItemAppendObject(
    note.get_handle(), ITEM_SUMMARY, converted.c_str(), converted.size(), bid, lock.size(), TRUE
  );
  result.throw_if_error("Failed to append attached object to note");
  (void)lock.release();
}

void attached_object::reallocate(size_t size) {
  const dmn::status result = NSFDbReallocObject(db_.get_handle(), hdl_.get(), size);
  result.throw_if_error("Failed to reallocate attached object memory");
}

void attached_object::write(detail::locker locker) {
  if (locker.get_ownership() != detail::ownership::take) {
    throw dmn::invalid_argument("Locker must own memory in order to write to attached object");
  }

  const dmn::status result =
    NSFDbWriteObject(db_.get_handle(), hdl_.get(), locker.get_handle(), 0, locker.size());
  result.throw_if_error("Failed to write data to attachted object");
  (void)locker.release();
}

auto attached_object::size() const -> size_t {
  DWORD size = 0;
  const dmn::status result = NSFDbGetObjectSize(
    db_.get_handle(), hdl_.get(), static_cast<uint16_t>(object_type_), &size, nullptr, nullptr
  );
  result.throw_if_error("Failed to obtain attached object size");
  return size;
}