#include "dmn/note_lock.hpp"

#include <array>

#include <domino/global.h>
#include <domino/nsfnote.h>
#include <domino/kfm.h>

#include "dmn/error.hpp"

using dmn::note_lock;

namespace {
auto get_db_handle(const dmn::database& db) noexcept -> std::optional<dmn::database::handle_t> {
  try {
    return db.get_handle();
  } catch (...) {
    return std::nullopt;
  }
}
}  // namespace

auto note_lock::acquire(dmn::database db, dmn::note_id noteid) -> note_lock {
  const dmn::status result = lock_note(db.get_handle(), noteid);
  if (result.is_locked()) {
    throw dmn::native_error("Note is already locked", result);
  }
  result.throw_if_error("Failed to acquire lock");

  return {std::move(db), noteid};
}

auto note_lock::try_acquire(dmn::database db, dmn::note_id noteid) noexcept
  -> std::optional<note_lock> {
  const auto db_handle = get_db_handle(db);
  const dmn::status result = lock_note(*db_handle, noteid);
  if (result.is_error()) {
    return std::nullopt;
  }

  return note_lock(std::move(db), noteid);
}

auto note_lock::try_unlock() noexcept -> bool {
  if (!owns_lock_) {
    return true;
  }

  const auto db_handle = get_db_handle(db_);
  const dmn::status result = NSFDbNoteUnlock(*db_handle, noteid_.value, NOTE_LOCK_HARD);
  if (result.is_error()) {
    return false;
  }

  owns_lock_ = false;
  return true;
}

void note_lock::unlock() {
  if (!owns_lock_) {
    return;
  }

  const dmn::status result = NSFDbNoteUnlock(db_.get_handle(), noteid_.value, NOTE_LOCK_HARD);
  result.throw_if_error("Failed to unlock note");

  owns_lock_ = false;
}

auto note_lock::lock_note(dmn::database::handle_t db_handle, dmn::note_id note_id) noexcept
  -> dmn::status {
  std::array<char, MAXUSERNAME> username{};
  dmn::status result = SECKFMGetUserName(username.data());
  if (result.is_error()) {
    return result;
  }

  return NSFDbNoteLock(db_handle, note_id.value, NOTE_LOCK_HARD, username.data(), nullptr, nullptr);
}
