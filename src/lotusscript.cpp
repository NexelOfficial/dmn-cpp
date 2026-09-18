#include "dmn/lotusscript.hpp"

#include <domino/global.h>
#include <domino/agents.h>
#include <domino/osmem.h>

#include "dmn/detail/locker.hpp"
#include "dmn/error.hpp"
#include "dmn/lmbcs.hpp"

using dmn::lotusscript;

lotusscript::lotusscript(std::string_view code) : hdl_(OSMemFree) {
  detail::dhandle_t dest_hdl{};
  detail::dhandle_t error_hdl{};
  {
    const auto converted = dmn::lmbcs::from_string(code);
    const std::span data{converted.c_str(), converted.size() + 1};
    auto lock = detail::locker::allocate(data);

    const dmn::status error =
      AgentLSTextFormat(lock.get_handle(), &dest_hdl, &error_hdl, 0, nullptr);
    error.throw_if_error("Failed to format LotusScript code");
  }

  if (error_hdl != detail::dhandle_t{}) {
    auto lock = detail::locker{error_hdl};
    auto* error_ptr = lock.get_pointer<char>();

    std::string error{error_ptr, std::strlen(error_ptr)};
    throw dmn::formatting_error(std::move(error));
  }

  auto lock = detail::locker{dest_hdl};
  auto* dest_ptr = lock.get_pointer<char>();

  size_ = std::strlen(dest_ptr);
  hdl_.put(lock.release().pool);
}