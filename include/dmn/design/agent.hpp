#pragma once

#include "dmn/detail/runtime.hpp"
#include "dmn/design/flags.hpp"
#include "dmn/lotusscript.hpp"
#include "dmn/database.hpp"
#include "dmn/formula.hpp"
#include "dmn/note.hpp"

namespace dmn::design {
/// Domino agent design element.
///
/// \throws dmn::invalid_handle If an underlying handle is empty.
/// \throws dmn::native_error In case of a lower level failure.
class agent : private detail::runtime {
 public:
  agent() = delete;

  /// Create an agent design element.
  ///
  /// \throws dmn::runtime_error If an agent with the title already exists.
  static auto create(const dmn::database& db, std::string_view title) -> agent;

  auto set_title(std::string_view title) -> agent&;
  auto set_comment(std::string_view comment) -> agent&;
  auto set_code(dmn::lotusscript code) -> agent&;
  auto set_code(dmn::formula code) -> agent&;
  auto set_trigger(design::trigger trig) -> agent&;

  [[nodiscard]] auto get_title() const -> std::string;
  [[nodiscard]] auto get_comment() const -> std::string;

  void save();

 private:
  dmn::note note_;

  void set_action_ex();
  void set_run_info();
  void set_assist_query();

  agent(dmn::note note);
};
}  // namespace dmn::design