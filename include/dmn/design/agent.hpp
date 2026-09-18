#pragma once

#include "dmn/design/flags.hpp"
#include "dmn/lotusscript.hpp"
#include "dmn/database.hpp"
#include "dmn/formula.hpp"
#include "dmn/note.hpp"

namespace dmn::design {
class agent {
 public:
  agent() = delete;

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