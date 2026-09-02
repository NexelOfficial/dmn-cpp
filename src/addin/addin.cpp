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
std::optional<dmn::lmbcs> addin::prefix_ = std::nullopt;

namespace {
auto to_escaped_lmbcs(std::string_view text) -> dmn::lmbcs {
  std::string escaped;
  escaped.reserve(text.size());
  for (const auto& chr : text) {
    if (chr == '%') {
      escaped += "%%";
    } else {
      escaped += chr;
    }
  }

  return dmn::lmbcs::from_string(escaped);
}
}  // namespace

auto addin::create(std::string_view name) -> addin {
  (void)dmn::session::instance();

  HMODULE module_handle{};
  detail::dhandle_t temp_status_line{};
  AddInQueryDefaults(&module_handle, &temp_status_line);
  AddInDeleteStatusLine(temp_status_line);

  auto converted = dmn::lmbcs::from_string(name);
  const detail::dhandle_t status_line = AddInCreateStatusLine(converted.data());
  if (status_line == detail::dhandle_t{}) {
    throw dmn::runtime_error("Failed to create status line");
  }

  AddInSetDefaults(module_handle, status_line);
  update_status("Idle");

  return {status_line};
}

void addin::update_status(std::string_view message) {
  auto converted = dmn::lmbcs::from_string(message);
  AddInSetStatusText(converted.data());
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

void addin::set_log_prefix(std::string_view prefix) {
  auto converted = to_escaped_lmbcs(prefix);
  prefix_.emplace(std::move(converted));
}

void addin::log_impl(std::string_view text) {
  auto message = prefix_.value_or({}) + to_escaped_lmbcs(text);
  AddInLogMessageText(reinterpret_cast<char*>(message.data()), NOERROR);
}

addin::addin(detail::dhandle_t handle) : status_hdl_(handle, AddInDeleteStatusLine) {};