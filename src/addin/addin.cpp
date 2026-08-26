#include "dmn/addin/addin.hpp"

#include <domino/global.h>
#include <domino/addin.h>
#include <domino/osmisc.h>

#include <exception>

#include "dmn/addin/session.hpp"
#include "dmn/addin/messages.hpp"
#include "dmn/error.hpp"

using dmn::addin;

std::mutex addin::mtx = {};

auto addin::create(std::string_view name) -> addin {
  (void)dmn::session::instance();

  HMODULE module_handle{};
  detail::dhandle_t temp_status_line{};
  AddInQueryDefaults(&module_handle, &temp_status_line);
  AddInDeleteStatusLine(temp_status_line);

  lmbcs::str converted = lmbcs::translate(name);
  const detail::dhandle_t status_line = AddInCreateStatusLine(lmbcs::cast(converted));
  if (status_line == detail::dhandle_t{}) {
    throw dmn::runtime_error("Failed to create status line");
  }

  AddInSetDefaults(module_handle, status_line);
  update_status("Idle");

  return {status_line};
}

void addin::update_status(const std::string& message) {
  lmbcs::str converted = lmbcs::translate(message);
  AddInSetStatusText(lmbcs::cast(converted));
}

auto addin::minutes_elapsed(size_t minutes) -> bool {
  return AddInMinutesHaveElapsed(minutes) != FALSE;
}

auto addin::seconds_elapsed(size_t minutes) -> bool {
  return AddInSecondsHaveElapsed(minutes) != FALSE;
}

auto addin::day_elapsed() -> bool { return AddInDayHasElapsed() != FALSE; }

void addin::enter_loop(const function_t& callback) const {
  if (!status_hdl_) {
    throw dmn::invalid_handle("Status line or message queue invalid");
  }

  const std::scoped_lock lock(mtx);

  while (AddInIdle() == FALSE) {
    OSPreemptOccasionally();

    try {
      callback();
    } catch (const std::exception& ex) {
      dmn::addin::log("An error was thrown from the add-in loop: ", ex.what());
      break;
    }

    if (AddInShouldTerminate() == TRUE) {
      break;
    }
  }

  dmn::addin::log("Add-in loop stopped.");
}

void addin::set_log_prefix(std::string_view prefix) { prefix_.emplace(prefix); }

void addin::log_impl(const std::string& text) {
  // Escape percent signs
  std::string escaped;
  escaped.reserve(text.size());
  for (const auto& chr : text) {
    if (chr == '%') {
      escaped += "%%";
    } else {
      escaped += chr;
    }
  }

  lmbcs::str converted = lmbcs::translate(escaped);
  AddInLogMessageText(lmbcs::cast(converted), dmn::no_error.value);
}

addin::addin(detail::dhandle_t handle) : status_hdl_(handle, AddInDeleteStatusLine) {};