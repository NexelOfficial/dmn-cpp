#pragma once

#include <utility>

#include "dmn/database.hpp"
#include "dmn/error.hpp"

namespace dmn {
class note_lock {
 public:
  note_lock() = delete;
  note_lock(dmn::database db, dmn::note_id noteid) noexcept
      : owns_lock_(true), db_(std::move(db)), noteid_(noteid) {}
  ~note_lock() noexcept { (void)try_unlock(); }

  note_lock(note_lock&& other) noexcept
      : owns_lock_(std::exchange(other.owns_lock_, false)),
        db_(std::move(other.db_)),
        noteid_(std::exchange(other.noteid_, 0)) {}

  auto operator=(note_lock&& other) = delete;
  note_lock(const note_lock& other) = delete;
  auto operator=(const note_lock& other) = delete;

  /// Acquire an exclusive lock for a note.
  ///
  /// \param db Database containing the note to lock.
  /// \param noteid Note ID of the note to lock.
  /// \return A note_lock instance owning the acquired lock.
  /// \throws dmn::error If the note is already locked or can't be acquired.
  /// \throws std::runtime_error If the underlying handle is empty.
  [[nodiscard]] static auto acquire(dmn::database db, dmn::note_id noteid) -> note_lock;

  /// Try to acquire an exclusive lock for a note.
  ///
  /// \param db Database containing the note to lock.
  /// \param noteid Note ID of the note to lock.
  /// \return A note_lock instance if the lock was acquired successfully, if available.
  [[nodiscard]] static auto try_acquire(dmn::database db, dmn::note_id noteid) noexcept
    -> std::optional<note_lock>;

  /// Try to release the owned note lock.
  ///
  /// \return true if the lock is not owned or was released successfully; otherwise false.
  /// \note Returns false if the database handle is unavailable or the unlock operation fails.
  [[nodiscard]] auto try_unlock() noexcept -> bool;

  /// Release the owned note lock.
  ///
  /// \throws dmn::error If the note cannot be unlocked.
  /// \throws std::runtime_error If the underlying handle is empty.
  void unlock();

 private:
  static_assert(std::is_nothrow_move_constructible_v<dmn::database>);

  dmn::database db_;
  bool owns_lock_;
  dmn::note_id noteid_;

  [[nodiscard]] static auto lock_note(
    dmn::database::handle_t db_handle, dmn::note_id note_id
  ) noexcept -> dmn::status;
};

}  // namespace dmn
