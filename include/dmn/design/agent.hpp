#pragma once

#include "dmn/database.hpp"
#include "dmn/note.hpp"

namespace dmn::design {
class agent {
 public:
  agent() = delete;

  static auto create(const dmn::database& db, std::string_view title) -> agent;

  void set_code(dmn::formula code);
  void save();

 private:
  dmn::note note_;

  void set_action_ex();
  void set_run_info();
  void set_assist_query();

  agent(dmn::note note);
};
}  // namespace dmn::design