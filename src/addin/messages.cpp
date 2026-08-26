#include "dmn/addin/messages.hpp"

#include <domino/global.h>
#include <domino/stdnames.h>
#include <domino/names.h>
#include <domino/mq.h>
#include <format>

#include "dmn/addin/session.hpp"
#include "dmn/error.hpp"

using dmn::messages;

static_assert(sizeof(messages::handle_t) == sizeof(MQHANDLE));

auto messages::open(std::string_view name) -> messages {
  (void)dmn::session::instance();

  handle_t handle = {};
  const std::string final_name = std::format("{}{}", TASK_QUEUE_PREFIX, name);
  const lmbcs::str converted = lmbcs::translate(final_name);
  const dmn::status result = MQOpen(lmbcs::cast(converted), MQ_OPEN_CREATE, &handle);
  result.throw_if_error("Failed to create MQ");

  return {handle};
}

auto messages::get_message() const -> std::optional<std::string> {
  std::string output(MAXPATH, '\0');
  uint16_t message_size = 0;

  const dmn::status result = MQGet(hdl_.get(), output.data(), MAXPATH, 0, 0, &message_size);
  if (!result.is_error()) {
    output.resize(message_size);
    return output;
  }

  return std::nullopt;
}

messages::messages(handle_t handle) : hdl_(handle, [](auto hdl) { MQClose(hdl, 0); }) {}