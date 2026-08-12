#pragma once

#include <string>
#include <optional>

#include "dmn/database.hpp"
#include "dmn/note.hpp"

namespace dmn {

class agent {
 public:
  using handle_t = void*;
  agent() = delete;

  /// Run the agent and capture its output.
  ///
  /// Optionally provides a note as the agent's document context and redirects
  /// agent output to an in-memory buffer.
  ///
  /// \param note Note to use as the agent context.
  /// \return Captured agent output. Returns an empty string if the agent produces no output, or an
  /// empty result if the output cannot be retrieved.
  /// \throws dmn::native_error If the run context cannot be created, the document context cannot be
  /// set, output redirection fails, or agent execution fails.
  /// \throws dmn::invalid_handle If the underlying agent handle is empty.
  [[nodiscard]] auto run(const std::optional<dmn::note>& note) const -> std::optional<std::string>;

 private:
  dmn::database db_;
  detail::uhandle<handle_t> hdl_;

  agent(dmn::database db, handle_t handle);

  /// Internal implementation used by `dmn::database`.
  static auto open(const dmn::database& db, std::string_view name) -> std::optional<agent>;

  friend class database;
};
}  // namespace dmn